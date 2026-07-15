#include "clock/IClock.h"
#include "controller/ApprovalController.h"
#include "controller/IInputReader.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IApprovalView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockApprovalView : public IApprovalView
{
    public:
        MOCK_METHOD(void, showReservedList, (const std::vector<ApprovalOrderRow>& rows), (override));
        MOCK_METHOD(void, showSufficientStockCheck, (const ApprovalOrderRow& row, int available_stock), (override));
        MOCK_METHOD(void, showInsufficientStockCheck,
            (const ApprovalOrderRow& row, int available_stock, int shortage_quantity,
                int real_production_quantity, double total_production_time_min),
            (override));
        MOCK_METHOD(void, showApprovalResult, (const Order& order), (override));
        MOCK_METHOD(void, showRejection, (const Order& order), (override));
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
    Order makeReservedOrder(const std::string& order_number, const std::string& sample_id, int quantity)
    {
        return Order{ order_number, sample_id, "고객", quantity, OrderStatus::RESERVED, 0, 0 };
    }
}

TEST(ApprovalControllerTest, SufficientStockApprovalDeductsAvailableStockOnly)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeReservedOrder("ORD-0001", "S-001", 100) }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.9, 480, 480 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("Y"));

    Sample captured_sample;
    Order captured_order;
    EXPECT_CALL(sample_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_sample), ReturnArg<0>()));
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_order), ReturnArg<0>()));

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured_sample.available_stock, 380);
    EXPECT_EQ(captured_sample.physical_stock, 480);
    EXPECT_EQ(captured_order.status, OrderStatus::CONFIRMED);
}

TEST(ApprovalControllerTest, InsufficientStockApprovalClampsAvailableStockToZeroAndRecordsShortage)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeReservedOrder("ORD-0002", "S-003", 200) }));
    ON_CALL(sample_repository, findById("S-003"))
        .WillByDefault(Return(Sample{ "S-003", "SiC 파워기판", 0.8, 0.92, 30, 30 }));
    ON_CALL(clock, nowMillis()).WillByDefault(Return(1776300000000LL));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("Y"));

    Sample captured_sample;
    Order captured_order;
    EXPECT_CALL(sample_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_sample), ReturnArg<0>()));
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_order), ReturnArg<0>()));

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured_sample.available_stock, 0);
    EXPECT_EQ(captured_sample.physical_stock, 30);
    EXPECT_EQ(captured_order.status, OrderStatus::PRODUCING);
    EXPECT_EQ(captured_order.shortage_quantity, 170);
    EXPECT_EQ(captured_order.enqueued_at_millis, 1776300000000LL);
}

TEST(ApprovalControllerTest, RejectionDoesNotTouchSampleStock)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeReservedOrder("ORD-0003", "S-001", 50) }));
    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼", 0.5, 0.9, 480, 480 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("N"));

    EXPECT_CALL(sample_repository, save(_)).Times(0);

    Order captured_order;
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&captured_order), ReturnArg<0>()));

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured_order.status, OrderStatus::REJECTED);
}

TEST(ApprovalControllerTest, SequentialApprovalsForSameSampleDoNotDoubleUseStock)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    // 재고 30, 순차로 주문1(수량 20), 주문2(수량 20) 승인.
    Sample sample{ "S-003", "SiC 파워기판", 0.8, 0.92, 30, 30 };

    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeReservedOrder("ORD-0001", "S-003", 20) }));
    ON_CALL(sample_repository, findById("S-003")).WillByDefault(ReturnPointee(&sample));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("Y"));
    ON_CALL(sample_repository, save(_)).WillByDefault(Invoke([&sample](const Sample& saved)
    {
        sample = saved;
        return saved;
    }));
    ON_CALL(order_repository, save(_)).WillByDefault(ReturnArg<0>());

    controller.processCommand("1");

    // 첫 주문(수량 20) 승인 후 남은 가용 재고는 10이어야 한다.
    EXPECT_EQ(sample.available_stock, 10);

    // 두 번째 주문(수량 20)은 남은 가용 재고 10만으로는 부족 → 부족분 10.
    ApprovalOrderRow second_row{ makeReservedOrder("ORD-0002", "S-003", 20), "SiC 파워기판" };
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ second_row.order }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("Y"));

    Order second_captured;
    EXPECT_CALL(order_repository, save(_)).WillOnce(DoAll(SaveArg<0>(&second_captured), ReturnArg<0>()));

    controller.processCommand("1");

    EXPECT_EQ(sample.available_stock, 0);
    EXPECT_EQ(second_captured.status, OrderStatus::PRODUCING);
    EXPECT_EQ(second_captured.shortage_quantity, 10);
}

TEST(ApprovalControllerTest, InvalidIndexShowsErrorAndDoesNotChangeAnything)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    // RESERVED 주문이 하나뿐인 상황에서 존재하지 않는 번호(5)를 선택하면 오류 처리되어야 한다.
    ON_CALL(order_repository, findAll())
        .WillByDefault(Return(std::vector<Order>{ makeReservedOrder("ORD-0001", "S-001", 10) }));

    EXPECT_CALL(order_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("5"));
}

TEST(ApprovalControllerTest, BackCommandStopsLoop)
{
    NiceMock<MockApprovalView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    ApprovalController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_FALSE(controller.processCommand("0"));
}
