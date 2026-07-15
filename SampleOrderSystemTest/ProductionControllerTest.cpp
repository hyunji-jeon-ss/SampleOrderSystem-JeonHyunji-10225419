#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ProductionController.h"
#include "production/ProductionQueueProcessor.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IProductionView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockProductionView : public IProductionView
{
    public:
        MOCK_METHOD(void, showLineStatus, (bool is_running, const std::string& current_time_text), (override));
        MOCK_METHOD(void, showActiveProduction, (const std::optional<ActiveProductionInfo>& active), (override));
        MOCK_METHOD(void, showQueue, (const std::vector<QueuedProductionInfo>& queue), (override));
        MOCK_METHOD(void, showMessage, (const std::string& message), (override));
};

class MockInputReader : public IInputReader
{
    public:
        MOCK_METHOD(std::string, readLine, (), (override));
};

class MockSampleRepository : public ISampleRepository
{
    public:
        MOCK_METHOD(Sample, save, (const Sample& sample), (override));
        MOCK_METHOD(std::optional<Sample>, findById, (const std::string& id), (override));
        MOCK_METHOD(std::vector<Sample>, findAll, (), (override));
};

class MockOrderRepository : public IOrderRepository
{
    public:
        MOCK_METHOD(Order, save, (const Order& order), (override));
        MOCK_METHOD(std::optional<Order>, findByOrderNumber, (const std::string& order_number), (override));
        MOCK_METHOD(std::vector<Order>, findAll, (), (override));
};

class MockClock : public IClock
{
    public:
        MOCK_METHOD(int64_t, nowMillis, (), (override));
};

namespace
{
    Order makeProducingOrder(const std::string& order_number, const std::string& sample_id,
        int quantity, int shortage_quantity, long long enqueued_at_millis)
    {
        Order order;
        order.order_number = order_number;
        order.sample_id = sample_id;
        order.customer_name = "고객";
        order.quantity = quantity;
        order.status = OrderStatus::PRODUCING;
        order.shortage_quantity = shortage_quantity;
        order.enqueued_at_millis = enqueued_at_millis;
        return order;
    }
}

TEST(ProductionControllerTest, ActiveProductionDisplaysProgressPercentAndDetails)
{
    NiceMock<MockProductionView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    ProductionController controller(view, input_reader, sample_repository, order_repository, queue_processor, clock);

    // 이미 시작된 주문: 시작 1000, 완료 9000, 현재(목 시계) 5000 -> 정확히 절반 지점(진행률 50%).
    Order active = makeProducingOrder("ORD-0001", "S-001", 80, 50, 1000);
    active.real_production_quantity = 55;
    active.production_start_millis = 1000;
    active.production_end_millis = 9000;

    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{ active }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "SiC 파워기판", 0.9, 0.92, 30, 30 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(5000LL));
    ON_CALL(order_repository, save(_)).WillByDefault(ReturnArg<0>());
    ON_CALL(sample_repository, save(_)).WillByDefault(ReturnArg<0>());
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("0"));

    std::optional<ActiveProductionInfo> captured;
    EXPECT_CALL(view, showActiveProduction(_)).WillOnce(SaveArg<0>(&captured));

    controller.run();

    ASSERT_TRUE(captured.has_value());
    EXPECT_EQ(captured->order.order_number, "ORD-0001");
    EXPECT_EQ(captured->available_stock_at_approval, 30);
    EXPECT_EQ(captured->progress_percent, 50);
}

TEST(ProductionControllerTest, EmptyQueueShowsNoneAndIdleStatus)
{
    NiceMock<MockProductionView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    ProductionController controller(view, input_reader, sample_repository, order_repository, queue_processor, clock);

    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{}));

    EXPECT_CALL(view, showLineStatus(false, _)).Times(1);
    EXPECT_CALL(view, showActiveProduction(Eq(std::nullopt))).Times(1);
    EXPECT_CALL(view, showQueue(IsEmpty())).Times(1);
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("0"));

    controller.run();
}

TEST(ProductionControllerTest, QueueDisplaysRemainingOrdersInFifoOrder)
{
    NiceMock<MockProductionView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    ProductionController controller(view, input_reader, sample_repository, order_repository, queue_processor, clock);

    // 활성 주문이 아직 완료되지 않은 상태(완료 시각이 목 시계보다 미래)여야 advanceQueue()가 매번 조회하는
    // findAll()의 정적(변화 없는) 목 데이터와 어긋나지 않는다. 완료로 만들면 두 번째 advanceQueue 호출에서도
    // 여전히 같은 완료 주문이 조회되어 무한 루프가 발생한다.
    Order active = makeProducingOrder("ORD-0001", "S-001", 80, 50, 1000);
    active.real_production_quantity = 54;
    active.production_start_millis = 1000;
    active.production_end_millis = 9000;

    Order queued = makeProducingOrder("ORD-0002", "S-001", 150, 150, 2000);

    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{ active, queued }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "SiC 파워기판", 0.5, 0.92, 30, 30 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(5000LL));
    ON_CALL(order_repository, save(_)).WillByDefault(ReturnArg<0>());
    ON_CALL(sample_repository, save(_)).WillByDefault(ReturnArg<0>());

    std::vector<QueuedProductionInfo> captured_queue;
    EXPECT_CALL(view, showQueue(_)).WillOnce(SaveArg<0>(&captured_queue));
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("0"));

    controller.run();

    ASSERT_EQ(captured_queue.size(), 1u);
    EXPECT_EQ(captured_queue[0].order.order_number, "ORD-0002");
    EXPECT_GT(captured_queue[0].expected_completion_millis, active.production_end_millis);
}

TEST(ProductionControllerTest, RefreshCommandLoopsAndAdvancesQueueAgain)
{
    NiceMock<MockProductionView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    ProductionController controller(view, input_reader, sample_repository, order_repository, queue_processor, clock);

    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{}));
    // advanceQueue()가 findAll을 1회, display()의 fetchProducingOrdersByFifo가 1회 호출 -> 새로고침 1회당 2회.
    EXPECT_CALL(order_repository, findAll()).Times(4);
    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("R"))
        .WillOnce(Return("0"));

    controller.run();
}

TEST(ProductionControllerTest, BackCommandStopsLoopImmediately)
{
    NiceMock<MockProductionView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    ProductionController controller(view, input_reader, sample_repository, order_repository, queue_processor, clock);

    EXPECT_FALSE(controller.processCommand("0"));
}
