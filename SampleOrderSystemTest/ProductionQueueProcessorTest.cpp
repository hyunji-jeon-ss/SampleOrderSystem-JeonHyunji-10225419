#include "clock/IClock.h"
#include "production/ProductionQueueProcessor.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

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

TEST(ProductionQueueProcessorTest, NotYetStartedOrderGetsStartAndEndTimesButStaysProducing)
{
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor processor(sample_repository, order_repository, clock);

    Order order = makeProducingOrder("ORD-0001", "S-001", 100, 50, 1000);
    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{ order }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 10.0, 1.0, 0, 0 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(2000LL));

    Order captured_order;
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_order), ReturnArg<0>()));
    EXPECT_CALL(sample_repository, save(_)).Times(0);

    processor.advanceQueue();

    EXPECT_EQ(captured_order.status, OrderStatus::PRODUCING);
    EXPECT_EQ(captured_order.real_production_quantity, 50);
    EXPECT_EQ(captured_order.production_start_millis, 2000LL);
    EXPECT_EQ(captured_order.production_end_millis, 2000LL + static_cast<long long>(50 * 10.0 * 60000.0));
}

TEST(ProductionQueueProcessorTest, CompletedOrderUpdatesPhysicalStockAndStatus)
{
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor processor(sample_repository, order_repository, clock);

    Order order = makeProducingOrder("ORD-0001", "S-001", 100, 50, 1000);
    order.real_production_quantity = 50;
    order.production_start_millis = 1000;
    order.production_end_millis = 5000;

    // findAll()이 매번 같은(변화 없는) 값을 반환하면 완료 처리 후에도 같은 주문이 계속 PRODUCING으로 조회되어
    // advanceQueue()의 while(true) 루프가 끝나지 않는다. save()로 상태가 반영된 벡터를 findAll()이 읽도록 한다.
    std::vector<Order> orders{ order };
    ON_CALL(order_repository, findAll()).WillByDefault(Invoke([&orders]() { return orders; }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 10.0, 1.0, 200, 0 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(5000LL));

    Sample captured_sample;
    Order captured_order;
    EXPECT_CALL(sample_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_sample), ReturnArg<0>()));
    EXPECT_CALL(order_repository, save(_)).WillOnce(Invoke([&orders, &captured_order](const Order& saved)
    {
        captured_order = saved;
        for (Order& existing : orders)
        {
            if (existing.order_number == saved.order_number) existing = saved;
        }
        return saved;
    }));

    processor.advanceQueue();

    EXPECT_EQ(captured_sample.physical_stock, 250);
    EXPECT_EQ(captured_order.status, OrderStatus::CONFIRMED);
}

TEST(ProductionQueueProcessorTest, BackdatedChainCompletesMultipleOrdersInOneCall)
{
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor processor(sample_repository, order_repository, clock);

    // 첫 번째 주문은 이미 시작되어 있고(완료 시각 5000), 두 번째 주문은 아직 시작 전.
    Order first = makeProducingOrder("ORD-0001", "S-001", 100, 50, 1000);
    first.real_production_quantity = 50;
    first.production_start_millis = 1000;
    first.production_end_millis = 5000;

    Order second = makeProducingOrder("ORD-0002", "S-001", 40, 20, 2000);

    std::vector<Order> orders{ first, second };

    ON_CALL(order_repository, findAll()).WillByDefault(Invoke([&orders]() { return orders; }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 2.0, 1.0, 200, 0 }));
    // 목 시계: 두 주문 모두 이미 완료됐어야 할 만큼 시간이 흐른 상태 (두 번째 주문은 20 ea * 2.0 min = 40 min = 2,400,000ms).
    ON_CALL(clock, nowMillis()).WillByDefault(Return(10'000'000LL));

    ON_CALL(order_repository, save(_)).WillByDefault(Invoke([&orders](const Order& saved)
    {
        for (Order& existing : orders)
        {
            if (existing.order_number == saved.order_number) existing = saved;
        }
        return saved;
    }));
    ON_CALL(sample_repository, save(_)).WillByDefault(ReturnArg<0>());

    processor.advanceQueue();

    const Order updated_first = orders[0];
    const Order updated_second = orders[1];

    EXPECT_EQ(updated_first.status, OrderStatus::CONFIRMED);
    EXPECT_EQ(updated_second.status, OrderStatus::CONFIRMED);

    // 두 번째 주문의 시작 시각은 첫 번째 주문의 완료 시각을 그대로 이어받아야 한다.
    EXPECT_EQ(updated_second.production_start_millis, 5000LL);
    EXPECT_EQ(updated_second.production_end_millis, 5000LL + static_cast<long long>(20 * 2.0 * 60000.0));
}

TEST(ProductionQueueProcessorTest, StillProducingDoesNotChangeInventory)
{
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor processor(sample_repository, order_repository, clock);

    Order order = makeProducingOrder("ORD-0001", "S-001", 100, 50, 1000);
    order.real_production_quantity = 50;
    order.production_start_millis = 1000;
    order.production_end_millis = 5000;

    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{ order }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 10.0, 1.0, 200, 0 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(3000LL));

    EXPECT_CALL(sample_repository, save(_)).Times(0);
    EXPECT_CALL(order_repository, save(_)).Times(0);

    processor.advanceQueue();
}
