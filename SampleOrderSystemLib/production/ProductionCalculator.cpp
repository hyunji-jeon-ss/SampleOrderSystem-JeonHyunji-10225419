#include "production/ProductionCalculator.h"

#include <cmath>

int computeRealProductionQuantity(int shortage_quantity, double yield)
{
    if (shortage_quantity <= 0) return 0;
    if (yield <= 0.0) return shortage_quantity;

    return static_cast<int>(std::ceil(shortage_quantity / yield));
}

double computeTotalProductionTimeMin(int real_production_quantity, double avg_production_time_min)
{
    return static_cast<double>(real_production_quantity) * avg_production_time_min;
}
