#include "controller/IInputReader.h"
#include "controller/SampleController.h"
#include "repository/ISampleRepository.h"
#include "view/ISampleView.h"

#include "gmock/gmock.h"

#include <optional>
#include <string>
#include <vector>

using namespace testing;

class MockSampleView : public ISampleView
{
    public:
        MOCK_METHOD(void, showSampleMenu, (), (override));
        MOCK_METHOD(void, showSamples, (const std::vector<Sample>& samples), (override));
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

TEST(SampleControllerTest, BackCommandStopsLoop)
{
    MockSampleView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    EXPECT_FALSE(controller.processCommand("0"));
}

TEST(SampleControllerTest, RegisterCommandSavesSampleWithParsedValues)
{
    MockSampleView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("실리콘 웨이퍼-8인치"))
        .WillOnce(Return("500"))
        .WillOnce(Return("0.92"));

    Sample captured;
    EXPECT_CALL(sample_repository, save(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(Sample{ "S-001", "실리콘 웨이퍼-8인치", 500, 0.92, 0, 0 })));
    EXPECT_CALL(view, showMessage(_)).Times(AtLeast(1));

    EXPECT_TRUE(controller.processCommand("1"));

    EXPECT_EQ(captured.name, "실리콘 웨이퍼-8인치");
    EXPECT_EQ(captured.avg_production_time_ms, 500);
    EXPECT_DOUBLE_EQ(captured.yield, 0.92);
    EXPECT_EQ(captured.physical_stock, 0);
}

TEST(SampleControllerTest, RegisterCommandShowsErrorOnInvalidNumber)
{
    MockSampleView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    EXPECT_CALL(input_reader, readLine())
        .WillOnce(Return("이름"))
        .WillOnce(Return("숫자아님"))
        .WillOnce(Return("0.9"));

    EXPECT_CALL(sample_repository, save(_)).Times(0);
    EXPECT_CALL(view, showMessage(_)).Times(AtLeast(1));

    EXPECT_TRUE(controller.processCommand("1"));
}

TEST(SampleControllerTest, ListCommandShowsAllSamples)
{
    NiceMock<MockSampleView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{ Sample{ "S-001", "A", 1, 1.0, 10, 10 } }));
    EXPECT_CALL(view, showSamples(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("2"));
}

TEST(SampleControllerTest, SearchCommandFiltersByNameCaseInsensitive)
{
    NiceMock<MockSampleView> view;
    MockInputReader input_reader;
    NiceMock<MockSampleRepository> sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    ON_CALL(sample_repository, findAll())
        .WillByDefault(Return(std::vector<Sample>{
            Sample{ "S-001", "Wafer-Sample", 1, 1.0, 0, 0 },
            Sample{ "S-002", "Other", 1, 1.0, 0, 0 }
        }));
    EXPECT_CALL(input_reader, readLine()).WillOnce(Return("wafer"));

    std::vector<Sample> matched;
    EXPECT_CALL(view, showSamples(_)).WillOnce(SaveArg<0>(&matched));

    EXPECT_TRUE(controller.processCommand("3"));

    ASSERT_EQ(matched.size(), 1u);
    EXPECT_EQ(matched[0].id, "S-001");
}

TEST(SampleControllerTest, UnknownCommandShowsErrorMessage)
{
    MockSampleView view;
    MockInputReader input_reader;
    MockSampleRepository sample_repository;
    SampleController controller(view, input_reader, sample_repository);

    EXPECT_CALL(view, showMessage(_)).Times(1);

    EXPECT_TRUE(controller.processCommand("9"));
}
