#pragma once

#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IApprovalView.h"

#include <string>
#include <vector>

class ApprovalController : public ISubMenuController
{
    public:
        ApprovalController(IApprovalView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        std::vector<ApprovalOrderRow> buildReservedRows();
        void handleApprovalFlow(const ApprovalOrderRow& row);

        IApprovalView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
