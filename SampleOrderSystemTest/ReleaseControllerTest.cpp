#include "controller/IInputReader.h"
#include "controller/ReleaseController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IReleaseView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockReleaseView : public IReleaseView
{
    public:
        MOCK_METHOD(void, showConfirmedList, (const std::vector<ReleaseOrderRow>& rows), (override));
        MOCK_METHOD(void, showReleaseCheck, (const ReleaseOrderRow& row, int physical_stock), (override));
        MOCK_METHOD(void, showReleaseResult, (const Order& order), (override));
        MOCK_METHOD(void, showCancellation, (const Order& order), (override));
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

namespace
{
    Order makeConfirmedOrder(const std::string& order_number, const std::string& sample_id, int quantity)
    {
        Order order;
        order.order_number = order_number;
        order.sample_id = sample_id;
        order.customer_name = "고객";
        order.quantity = quantity;
        order.status = OrderStatus::CONFIRMED;
        return order;
    }
}

TEST(ReleaseControllerTest, NormalReleaseDeductsPhysicalStockByOrderQuantity)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeConfirmedOrder("ORD-0001", "S-001", 40) }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 480, 380 }));

    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("Y"));

    Sample captured_sample;
    Order captured_order;
    EXPECT_CALL(sample_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_sample), ReturnArg<0>()));
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_order), ReturnArg<0>()));

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured_sample.physical_stock, 440);
    EXPECT_EQ(captured_sample.available_stock, 380);
    EXPECT_EQ(captured_order.status, OrderStatus::RELEASED);
}

TEST(ReleaseControllerTest, ReleaseDeductsOrderQuantityNotRealProductionQuantity)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    // 수율 보정으로 실생산량(46)이 주문 수량(40)보다 많이 반영된 상태에서 출고.
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeConfirmedOrder("ORD-0002", "S-005", 40) }));
    ON_CALL(sample_repository, findById("S-005"))
        .WillByDefault(Return(Sample{ "S-005", "산화막 웨이퍼-SiO2", 0.05, 0.88, 46, 0 }));

    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("Y"));

    Sample captured_sample;
    EXPECT_CALL(sample_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_sample), ReturnArg<0>()));
    EXPECT_CALL(order_repository, save(_)).WillOnce(ReturnArg<0>());

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured_sample.physical_stock, 6);
}

TEST(ReleaseControllerTest, CancellationDoesNotTouchStockOrStatus)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeConfirmedOrder("ORD-0003", "S-001", 40) }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.92, 480, 380 }));

    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("N"));

    EXPECT_CALL(sample_repository, save(_)).Times(0);
    EXPECT_CALL(order_repository, save(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("1"));
}

TEST(ReleaseControllerTest, ListOnlyIncludesConfirmedOrders)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    Order reserved = makeConfirmedOrder("ORD-0001", "S-001", 10);
    reserved.status = OrderStatus::RESERVED;
    Order producing = makeConfirmedOrder("ORD-0002", "S-001", 20);
    producing.status = OrderStatus::PRODUCING;
    Order confirmed = makeConfirmedOrder("ORD-0003", "S-001", 30);
    Order rejected = makeConfirmedOrder("ORD-0004", "S-001", 40);
    rejected.status = OrderStatus::REJECTED;
    Order released = makeConfirmedOrder("ORD-0005", "S-001", 50);
    released.status = OrderStatus::RELEASED;

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ reserved, producing, confirmed, rejected, released }));

    std::vector<ReleaseOrderRow> captured_rows;
    EXPECT_CALL(view, showConfirmedList(_)).WillOnce(SaveArg<0>(&captured_rows));
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("0"));

    controller.run();

    ASSERT_EQ(captured_rows.size(), 1u);
    EXPECT_EQ(captured_rows[0].order.order_number, "ORD-0003");
}

TEST(ReleaseControllerTest, InvalidIndexShowsErrorAndDoesNotChangeAnything)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeConfirmedOrder("ORD-0001", "S-001", 10) }));

    EXPECT_CALL(order_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("5"));
}

TEST(ReleaseControllerTest, BackCommandStopsLoop)
{
    NiceMock<MockReleaseView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    ReleaseController controller(view, input_reader, sample_repository, order_repository);

    EXPECT_FALSE(controller.processCommand("0"));
}
