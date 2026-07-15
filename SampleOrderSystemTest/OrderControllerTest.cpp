#include "controller/IInputReader.h"
#include "controller/OrderController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IOrderView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockOrderView : public IOrderView
{
    public:
        MOCK_METHOD(void, showConfirmation, (const Sample& sample, const std::string& customer_name, int quantity), (override));
        MOCK_METHOD(void, showReservationComplete, (const Order& order), (override));
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

TEST(OrderControllerTest, ConfirmedReservationIsSavedAsReserved)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "실리콘 웨이퍼-8인치", 0.1, 0.92, 100, 100 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("S-001"))
        .WillOnce(Return("삼성전자 파운드리"))
        .WillOnce(Return("200"))
        .WillOnce(Return("Y"));

    Order captured;
    EXPECT_CALL(order_repository, save(_))
        .WillOnce(DoAll(SaveArg<0>(&captured),
            Return(Order{ "ORD-20260416-0043", "S-001", "삼성전자 파운드리", 200, OrderStatus::RESERVED })));
    EXPECT_CALL(view, showReservationComplete(_)).Times(1);

    EXPECT_TRUE(controller.processReservation());

    EXPECT_EQ(captured.sample_id, "S-001");
    EXPECT_EQ(captured.customer_name, "삼성전자 파운드리");
    EXPECT_EQ(captured.quantity, 200);
    EXPECT_EQ(captured.status, OrderStatus::RESERVED);
}

TEST(OrderControllerTest, CancelledReservationIsNotSaved)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "A", 0.1, 1.0, 100, 100 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("S-001"))
        .WillOnce(Return("고객A"))
        .WillOnce(Return("10"))
        .WillOnce(Return("N"));

    EXPECT_CALL(order_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(AtLeast(1));

    EXPECT_FALSE(controller.processReservation());
}

TEST(OrderControllerTest, UnknownSampleIdStopsBeforeConfirmation)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById("S-999")).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("S-999"));

    EXPECT_CALL(view, showConfirmation(_, _, _)).Times(0);
    EXPECT_CALL(order_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(AtLeast(1));

    EXPECT_FALSE(controller.processReservation());
}

TEST(OrderControllerTest, InvalidQuantityStopsBeforeConfirmation)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "A", 0.1, 1.0, 100, 100 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("S-001"))
        .WillOnce(Return("고객A"))
        .WillOnce(Return("숫자아님"));

    EXPECT_CALL(view, showConfirmation(_, _, _)).Times(0);
    EXPECT_CALL(order_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(AtLeast(1));

    EXPECT_FALSE(controller.processReservation());
}

TEST(OrderControllerTest, ZeroQuantityIsRejected)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById("S-001"))
        .WillByDefault(Return(Sample{ "S-001", "A", 0.1, 1.0, 100, 100 }));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("S-001"))
        .WillOnce(Return("고객A"))
        .WillOnce(Return("0"));

    EXPECT_CALL(order_repository, save(_)).Times(0);

    EXPECT_FALSE(controller.processReservation());
}

TEST(OrderControllerTest, RunPausesForBackAfterProcessing)
{
    NiceMock<MockOrderView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    OrderController controller(view, input_reader, sample_repository, order_repository);

    ON_CALL(sample_repository, findById(_)).WillByDefault(Return(std::nullopt));

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("S-999"))
        .WillOnce(Return(""));

    controller.run();
}
