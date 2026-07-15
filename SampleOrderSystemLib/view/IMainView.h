#pragma once

#include <string>

struct MainMenuSummary
{
    std::string current_time_text;
    int sample_count = 0;
    int total_stock = 0;
    int order_count = 0;
    int production_queue_count = 0;
};

class IMainView
{
    public:
        virtual ~IMainView() = default;

        virtual void showMainMenu(const MainMenuSummary& summary) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
