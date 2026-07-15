#pragma once

#include "view/IMonitoringView.h"

class ConsoleMonitoringView : public IMonitoringView
{
    public:
        void showMenu(const std::string& current_time_text) override;
        void showOrderStatus(const OrderStatusSummary& order_summary) override;
        void showStockStatus(const std::vector<StockStatusRow>& stock_rows) override;
        void showMessage(const std::string& message) override;
};
