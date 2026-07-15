#pragma once

#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IMonitoringView.h"

#include <string>

class MonitoringController : public ISubMenuController
{
    public:
        MonitoringController(IMonitoringView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        void display();
        OrderStatusSummary buildOrderStatusSummary();
        std::vector<StockStatusRow> buildStockStatusRows();

        IMonitoringView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
