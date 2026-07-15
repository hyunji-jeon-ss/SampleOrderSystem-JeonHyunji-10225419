#pragma once

#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IReleaseView.h"

#include <string>
#include <vector>

class ReleaseController : public ISubMenuController
{
    public:
        ReleaseController(IReleaseView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        std::vector<ReleaseOrderRow> buildConfirmedRows();
        void handleReleaseFlow(const ReleaseOrderRow& row);

        IReleaseView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
};
