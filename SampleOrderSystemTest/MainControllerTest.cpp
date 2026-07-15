#include "controller/IInputReader.h"
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

TEST(MainControllerTest, ExitCommandStopsLoop)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MainController controller(view, input_reader, sample_repository, order_repository);

    EXPECT_FALSE(controller.processCommand("0"));
}

TEST(MainControllerTest, UnimplementedMenuShowsPlaceholderMessage)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MainController controller(view, input_reader, sample_repository, order_repository);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("1"));
}

TEST(MainControllerTest, UnknownCommandShowsErrorMessage)
{
    MockMainView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    MockOrderRepository order_repository;
    MainController controller(view, input_reader, sample_repository, order_repository);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("9"));
}
