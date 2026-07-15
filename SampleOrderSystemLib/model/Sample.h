#pragma once

#include <string>

struct Sample
{
    std::string id;
    std::string name;
    long long avg_production_time_ms = 0;
    double yield = 1.0;
};
