#pragma once

#include "view/IProductionView.h"

class ConsoleProductionView : public IProductionView
{
    public:
        void showLineStatus(bool is_running, const std::string& current_time_text) override;
        void showActiveProduction(const std::optional<ActiveProductionInfo>& active) override;
        void showQueue(const std::vector<QueuedProductionInfo>& queue) override;
        void showMessage(const std::string& message) override;
};
