#pragma once

#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IMainView.h"

#include <string>

class MainController
{
    public:
        MainController(IMainView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock,
            ISubMenuController* sample_menu = nullptr, ISubMenuController* order_menu = nullptr,
            ISubMenuController* approval_menu = nullptr);

        void run();
        bool processCommand(const std::string& command);

    private:
        MainMenuSummary buildSummary();

        IMainView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
        ISubMenuController* sample_menu;
        ISubMenuController* order_menu;
        ISubMenuController* approval_menu;
};
