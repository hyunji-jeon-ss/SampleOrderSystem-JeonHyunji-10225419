#pragma once

#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/ISampleRepository.h"
#include "view/ISampleView.h"

#include <string>
#include <vector>

class SampleController : public ISubMenuController
{
    public:
        SampleController(ISampleView& view, IInputReader& input_reader, ISampleRepository& sample_repository);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        void handleRegister();
        void handleList();
        void handleSearch();
        void displayPaged(const std::vector<Sample>& samples);

        ISampleView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
};
