#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/MonitoringController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IMonitoringView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockMonitoringView : public IMonitoringView
{
    public:
        MOCK_METHOD(void, showMenu, (const std::string& current_time_text), (override));
        MOCK_METHOD(void, showOrderStatus, (const OrderStatusSummary& order_summary), (override));
        MOCK_METHOD(void, showStockStatus, (const std::vector<StockStatusRow>& stock_rows), (override));
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
    Order makeOrder(const std::string& order_number, const std::string& sample_id, int quantity, OrderStatus status)
    {
        Order order;
        order.order_number = order_number;
        order.sample_id = sample_id;
        order.customer_name = "고객";
        order.quantity = quantity;
        order.status = status;
        return order;
    }
}

TEST(MonitoringControllerTest, OrderStatusSummaryCountsEachStatusAndExcludesRejected)
{
    NiceMock<MockMonitoringView> view;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MockInputReader input_reader;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{
            makeOrder("ORD-0001", "S-001", 10, OrderStatus::RESERVED),
            makeOrder("ORD-0002", "S-001", 10, OrderStatus::RESERVED),
            makeOrder("ORD-0003", "S-001", 10, OrderStatus::CONFIRMED),
            makeOrder("ORD-0004", "S-001", 10, OrderStatus::PRODUCING),
            makeOrder("ORD-0005", "S-001", 10, OrderStatus::RELEASED),
            makeOrder("ORD-0006", "S-001", 10, OrderStatus::REJECTED) }));

    OrderStatusSummary captured;
    EXPECT_CALL(view, showOrderStatus(_)).WillOnce(SaveArg<0>(&captured));
    EXPECT_CALL(view, showStockStatus(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured.reserved_count, 2);
    EXPECT_EQ(captured.confirmed_count, 1);
    EXPECT_EQ(captured.producing_count, 1);
    EXPECT_EQ(captured.released_count, 1);
}

TEST(MonitoringControllerTest, StockStatusSufficientWhenStockCoversPendingDemand)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{ Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 100, 100 } }));
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeOrder("ORD-0001", "S-001", 30, OrderStatus::RESERVED) }));

    std::vector<StockStatusRow> captured;
    EXPECT_CALL(view, showStockStatus(_)).WillOnce(SaveArg<0>(&captured));
    EXPECT_CALL(view, showOrderStatus(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("2"));

    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].pending_demand, 30);
    EXPECT_EQ(captured[0].status, StockStatus::SUFFICIENT);
}

TEST(MonitoringControllerTest, StockStatusShortageWhenStockBelowPendingDemand)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{ Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 20, 20 } }));
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{
            makeOrder("ORD-0001", "S-001", 15, OrderStatus::PRODUCING),
            makeOrder("ORD-0002", "S-001", 10, OrderStatus::CONFIRMED) }));

    std::vector<StockStatusRow> captured;
    EXPECT_CALL(view, showStockStatus(_)).WillOnce(SaveArg<0>(&captured));

    EXPECT_TRUE(controller.processCommand("2"));

    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].pending_demand, 25);
    EXPECT_EQ(captured[0].status, StockStatus::SHORTAGE);
}

TEST(MonitoringControllerTest, StockStatusDepletedWhenPhysicalStockIsZeroRegardlessOfDemand)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{ Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 0, 0 } }));
    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{}));

    std::vector<StockStatusRow> captured;
    EXPECT_CALL(view, showStockStatus(_)).WillOnce(SaveArg<0>(&captured));

    EXPECT_TRUE(controller.processCommand("2"));

    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].pending_demand, 0);
    EXPECT_EQ(captured[0].status, StockStatus::DEPLETED);
}

TEST(MonitoringControllerTest, PendingDemandExcludesRejectedAndReleasedOrders)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{ Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 10, 10 } }));
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{
            makeOrder("ORD-0001", "S-001", 100, OrderStatus::REJECTED),
            makeOrder("ORD-0002", "S-001", 200, OrderStatus::RELEASED),
            makeOrder("ORD-0003", "S-001", 5, OrderStatus::RESERVED) }));

    std::vector<StockStatusRow> captured;
    EXPECT_CALL(view, showStockStatus(_)).WillOnce(SaveArg<0>(&captured));

    EXPECT_TRUE(controller.processCommand("2"));

    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].pending_demand, 5);
}

TEST(MonitoringControllerTest, EmptySampleListShowsEmptyStockRows)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(sample_repository, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    std::vector<StockStatusRow> captured;
    EXPECT_CALL(view, showStockStatus(_)).WillOnce(SaveArg<0>(&captured));

    EXPECT_TRUE(controller.processCommand("2"));

    EXPECT_TRUE(captured.empty());
}

TEST(MonitoringControllerTest, MenuLoopsShowingMenuEachIterationUntilBack)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMenu(_)).Times(3);
    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("1"))
        .WillOnce(Return("2"))
        .WillOnce(Return("0"));

    controller.run();
}

TEST(MonitoringControllerTest, UnknownCommandShowsErrorAndContinuesLoop)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("9"));
}

TEST(MonitoringControllerTest, BackCommandStopsLoopImmediately)
{
    NiceMock<MockMonitoringView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MonitoringController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_FALSE(controller.processCommand("0"));
}
