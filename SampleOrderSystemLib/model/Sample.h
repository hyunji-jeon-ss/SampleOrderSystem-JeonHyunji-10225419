#pragma once

#include <string>

struct Sample
{
    std::string id;
    std::string name;
    double avg_production_time_min = 0.0;
    double yield = 1.0;
    int physical_stock = 0;
    int available_stock = 0;
};
