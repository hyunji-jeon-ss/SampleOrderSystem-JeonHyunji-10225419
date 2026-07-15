#pragma once

#include <string>
#include <vector>

struct OrderStatusSummary
{
    int reserved_count = 0;
    int confirmed_count = 0;
    int producing_count = 0;
    int released_count = 0;
};

enum class StockStatus
{
    SUFFICIENT,
    SHORTAGE,
    DEPLETED
};

struct StockStatusRow
{
    std::string sample_id;
    std::string sample_name;
    int physical_stock = 0;
    int pending_demand = 0;
    StockStatus status = StockStatus::SUFFICIENT;
};

class IMonitoringView
{
    public:
        virtual ~IMonitoringView() = default;

        virtual void showMenu(const std::string& current_time_text) = 0;
        virtual void showOrderStatus(const OrderStatusSummary& order_summary) = 0;
        virtual void showStockStatus(const std::vector<StockStatusRow>& stock_rows) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
