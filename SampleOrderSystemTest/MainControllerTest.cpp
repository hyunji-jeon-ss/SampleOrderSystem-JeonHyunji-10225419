#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "controller/MainController.h"
#include "production/ProductionQueueProcessor.h"
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

TEST(MainControllerTest, ApprovalMenuPlaceholderMessageWhenNotProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("3"));
}

TEST(MainControllerTest, ApprovalMenuCommandDelegatesToSubMenuControllerWhenProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MockSubMenuController approval_menu;
    MainController controller(view, input_reader, sample_repository, order_repository, clock,
        nullptr, nullptr, &approval_menu);

    EXPECT_CALL(approval_menu, run()).Times(1);
    EXPECT_CALL(view, showMessage(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("3"));
}

TEST(MainControllerTest, ProductionMenuPlaceholderMessageWhenNotProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("5"));
}

TEST(MainControllerTest, ProductionMenuCommandDelegatesToSubMenuControllerWhenProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MockSubMenuController production_menu;
    MainController controller(view, input_reader, sample_repository, order_repository, clock,
        nullptr, nullptr, nullptr, &production_menu);

    EXPECT_CALL(production_menu, run()).Times(1);
    EXPECT_CALL(view, showMessage(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("5"));
}

TEST(MainControllerTest, ProductionQueueProcessorAdvanceQueueCalledEachLoopIteration)
{
    MockMainView view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    MockOrderRepository order_repository;
    NiceMock<MockClock> clock;
    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);
    MainController controller(view, input_reader, sample_repository, order_repository, clock,
        nullptr, nullptr, nullptr, nullptr, &queue_processor);

    ON_CALL(sample_repository, findAll()).WillByDefault(Return(std::vector<Sample>{}));
    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("9"))
        .WillOnce(Return("0"));

    // 매 루프 반복마다 advanceQueue()(findAll 1회) + buildSummary()(findAll 1회) = 반복당 2회.
    // 2회 반복(첫 반복 "9" 처리 후 계속, 두 번째 반복 "0"으로 종료) -> 총 4회.
    EXPECT_CALL(order_repository, findAll())
        .Times(4)
        .WillRepeatedly(Return(std::vector<Order>{}));

    controller.run();
}

TEST(MainControllerTest, ReleaseMenuPlaceholderMessageWhenNotProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MainController controller(view, input_reader, sample_repository, order_repository, clock);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("6"));
}

TEST(MainControllerTest, ReleaseMenuCommandDelegatesToSubMenuControllerWhenProvided)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MockClock clock;
    MockSubMenuController release_menu;
    MainController controller(view, input_reader, sample_repository, order_repository, clock,
        nullptr, nullptr, nullptr, nullptr, nullptr, &release_menu);

    EXPECT_CALL(release_menu, run()).Times(1);
    EXPECT_CALL(view, showMessage(_)).Times(0);

    EXPECT_TRUE(controller.processCommand("6"));
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
