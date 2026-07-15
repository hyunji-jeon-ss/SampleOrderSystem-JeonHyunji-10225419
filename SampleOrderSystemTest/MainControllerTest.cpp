#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "controller/MainController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IMainView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockMainView : public IMainView
{
    public:
        MOCK_METHOD(void, showMainMenu, (const MainMenuSummary& summary), (override));
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

class MockSubMenuController : public ISubMenuController
{
    public:
        MOCK_METHOD(void, run, (), (override));
};

TEST(MainControllerTest, ExitCommandStopsLoop)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_FALSE(controller.processCommand("0"));
}

TEST(MainControllerTest, UnimplementedMenuShowsPlaceholderMessage)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("1"));
}

TEST(MainControllerTest, SampleMenuCommandDelegatesToSubMenuControllerWhenProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MockSubMenuController sample_menu;
    MainController controller(view, input_reader, sample_repository, order_repository, clock, &sample_menu);

    EXPECT_CALL(sample_menu, run()).Times(1);
    EXPECT_CALL(view, showMessage(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("1"));
}

TEST(MainControllerTest, OrderMenuPlaceholderMessageWhenNotProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("2"));
}

TEST(MainControllerTest, OrderMenuCommandDelegatesToSubMenuControllerWhenProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MockSubMenuController order_menu;
    MainController controller(view, input_reader, sample_repository, order_repository, clock, nullptr, &order_menu);

    EXPECT_CALL(order_menu, run()).Times(1);
    EXPECT_CALL(view, showMessage(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("2"));
}

TEST(MainControllerTest, UnknownCommandShowsErrorMessage)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("9"));
}

TEST(MainControllerTest, RunShowsMenuWithCurrentTimeAndExits)
{
    MockMainView view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    NiceMock<MockOrderRepository> order_repository;
    NiceMock<MockClock> clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    ON_CALL(clock, nowMillis()).WillByDefault(Return(1776300000000LL));
    ON_CALL(sample_repository, findAll()).WillByDefault(Return(std::vector<Sample>{}));
    ON_CALL(order_repository, findAll()).WillByDefault(Return(std::vector<Order>{}));
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("0"));

    MainMenuSummary captured_summary;
    EXPECT_CALL(view, showMainMenu(_))
        .WillOnce(SaveArg<0>(&captured_summary));

    controller.run();

    EXPECT_FALSE(captured_summary.current_time_text.empty());
}
