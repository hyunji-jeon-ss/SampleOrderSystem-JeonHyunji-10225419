#pragma once

#include "controller/IInputReader.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IMainView.h"

#include <string>

class MainController
{
    public:
        MainController(IMainView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run();
        bool processCommand(const std::string& command);

    private:
        MainMenuSummary buildSummary();

        IMainView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
};
