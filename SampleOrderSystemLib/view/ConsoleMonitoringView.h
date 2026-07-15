#pragma once

#include "view/IMonitoringView.h"

class ConsoleMonitoringView : public IMonitoringView
{
    public:
        void showMonitoring(const std::string& current_time_text,
            const OrderStatusSummary& order_summary, const std::vector<StockStatusRow>& stock_rows) override;
        void showMessage(const std::string& message) override;
};
