#include "repository/JsonOrderRepository.h"

#include "gmock/gmock.h"

#include <cstdio>
#include <string>

using namespace testing;

namespace
{
    constexpr int64_t kFixedDayMillis = 1776300000000LL;
}

class MockClock : public IClock
{
    public:
        MOCK_METHOD(int64_t, nowMillis, (), (override));
};

class JsonOrderRepositoryTest : public Test
{
    protected:
        void SetUp() override
        {
            std::remove(file_path.c_str());
        }

        void TearDown() override
        {
            std::remove(file_path.c_str());
        }

        std::string file_path = "test_orders.json";
        MockClock clock;
};

TEST_F(JsonOrderRepositoryTest, FindAllReturnsEmptyWhenFileDoesNotExist)
{
    ON_CALL(clock, nowMillis()).WillByDefault(Return(kFixedDayMillis));
    JsonOrderRepository repository(file_path, clock);

    EXPECT_TRUE(repository.findAll().empty());
}

TEST_F(JsonOrderRepositoryTest, SaveGeneratesOrderNumberWithDateAndSequence)
{
    EXPECT_CALL(clock, nowMillis()).WillRepeatedly(Return(kFixedDayMillis));
    JsonOrderRepository repository(file_path, clock);

    const Order saved = repository.save(Order{ "", "S-001", "삼성전자 파운드리", 200, OrderStatus::RESERVED });

    EXPECT_THAT(saved.order_number, StartsWith("ORD-"));
    EXPECT_THAT(saved.order_number, EndsWith("-0001"));
}

TEST_F(JsonOrderRepositoryTest, SecondSaveSameDayIncrementsSequence)
{
    EXPECT_CALL(clock, nowMillis()).WillRepeatedly(Return(kFixedDayMillis));
    JsonOrderRepository repository(file_path, clock);

    repository.save(Order{ "", "S-001", "고객A", 100, OrderStatus::RESERVED });
    const Order second = repository.save(Order{ "", "S-002", "고객B", 50, OrderStatus::RESERVED });

    EXPECT_THAT(second.order_number, EndsWith("-0002"));
}

TEST_F(JsonOrderRepositoryTest, DataSurvivesRepositoryRecreation)
{
    {
        EXPECT_CALL(clock, nowMillis()).WillRepeatedly(Return(kFixedDayMillis));
        JsonOrderRepository repository(file_path, clock);
        repository.save(Order{ "", "S-001", "고객A", 100, OrderStatus::RESERVED });
    }

    MockClock reload_clock;
    ON_CALL(reload_clock, nowMillis()).WillByDefault(Return(kFixedDayMillis));
    JsonOrderRepository reloaded(file_path, reload_clock);

    ASSERT_EQ(reloaded.findAll().size(), 1u);
    EXPECT_EQ(reloaded.findAll()[0].customer_name, "고객A");
}

TEST_F(JsonOrderRepositoryTest, FindByOrderNumberReturnsNulloptWhenMissing)
{
    ON_CALL(clock, nowMillis()).WillByDefault(Return(kFixedDayMillis));
    JsonOrderRepository repository(file_path, clock);

    EXPECT_FALSE(repository.findByOrderNumber("ORD-NOTFOUND-0001").has_value());
}

TEST_F(JsonOrderRepositoryTest, StatusPersistsAcrossReload)
{
    {
        EXPECT_CALL(clock, nowMillis()).WillRepeatedly(Return(kFixedDayMillis));
        JsonOrderRepository repository(file_path, clock);
        const Order saved = repository.save(Order{ "", "S-001", "고객A", 100, OrderStatus::RESERVED });

        Order updated = saved;
        updated.status = OrderStatus::CONFIRMED;
        repository.save(updated);
    }

    MockClock reload_clock;
    ON_CALL(reload_clock, nowMillis()).WillByDefault(Return(kFixedDayMillis));
    JsonOrderRepository reloaded(file_path, reload_clock);

    ASSERT_EQ(reloaded.findAll().size(), 1u);
    EXPECT_EQ(reloaded.findAll()[0].status, OrderStatus::CONFIRMED);
}
