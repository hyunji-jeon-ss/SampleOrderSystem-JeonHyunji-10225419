#pragma once

#include "controller/IInputReader.h"
#include "controller/ISubMenuController.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"
#include "view/IOrderView.h"

class OrderController : public ISubMenuController
{
    public:
        OrderController(IOrderView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run() override;
        bool processReservation();

    private:
        IOrderView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
};
