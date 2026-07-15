#pragma once

#include "model/Order.h"

#include <optional>
#include <string>
#include <vector>

struct ActiveProductionInfo
{
    Order order;
    std::string sample_name;
    int available_stock_at_approval = 0;
    double yield = 1.0;
    double total_production_time_min = 0.0;
    int progress_percent = 0;
};

struct QueuedProductionInfo
{
    Order order;
    std::string sample_name;
    int projected_real_production_quantity = 0;
    long long expected_completion_millis = 0;
};

class IProductionView
{
    public:
        virtual ~IProductionView() = default;

        virtual void showLineStatus(bool is_running) = 0;
        virtual void showActiveProduction(const std::optional<ActiveProductionInfo>& active) = 0;
        virtual void showQueue(const std::vector<QueuedProductionInfo>& queue) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
