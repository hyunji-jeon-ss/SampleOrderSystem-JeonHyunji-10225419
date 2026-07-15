#pragma once

#include "clock/IClock.h"
#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "production/ProductionQueueProcessor.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IProductionView.h"

#include <string>

class ProductionController : public ISubMenuController
{
    public:
        ProductionController(IProductionView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository,
            ProductionQueueProcessor& queue_processor, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        void display();

        IProductionView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        ProductionQueueProcessor& queue_processor;
        IClock& clock;
};
