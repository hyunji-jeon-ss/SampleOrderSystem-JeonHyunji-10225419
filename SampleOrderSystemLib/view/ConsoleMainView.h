#pragma once

#include "view/IMainView.h"

class ConsoleMainView : public IMainView
{
    public:
        void showMainMenu(const MainMenuSummary& summary) override;
        void showMessage(const std::string& message) override;
};
