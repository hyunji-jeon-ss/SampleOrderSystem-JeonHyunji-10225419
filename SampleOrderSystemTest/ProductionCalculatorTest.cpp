#include "production/ProductionCalculator.h"

#include "gmock/gmock.h"

using namespace testing;

TEST(ProductionCalculatorTest, ComputesCeilingOfShortageOverYield)
{
    EXPECT_EQ(computeRealProductionQuantity(170, 0.92), 185);
}

TEST(ProductionCalculatorTest, ExactDivisionDoesNotRoundUp)
{
    EXPECT_EQ(computeRealProductionQuantity(100, 0.5), 200);
}

TEST(ProductionCalculatorTest, ZeroShortageProducesZero)
{
    EXPECT_EQ(computeRealProductionQuantity(0, 0.9), 0);
}

TEST(ProductionCalculatorTest, NegativeShortageProducesZero)
{
    EXPECT_EQ(computeRealProductionQuantity(-10, 0.9), 0);
}

TEST(ProductionCalculatorTest, TotalProductionTimeMultipliesQuantityByAverageTime)
{
    EXPECT_DOUBLE_EQ(computeTotalProductionTimeMin(185, 0.8), 148.0);
}
