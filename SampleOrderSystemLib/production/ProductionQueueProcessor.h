#pragma once

#include "clock/IClock.h"
#include "model/Order.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"

#include <vector>

class ProductionQueueProcessor
{
    public:
        ProductionQueueProcessor(ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void advanceQueue();

    private:
        std::vector<Order> fetchProducingOrdersByFifo();

        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
