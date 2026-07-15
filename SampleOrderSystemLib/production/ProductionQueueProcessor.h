#pragma once

#include "clock/IClock.h"
#include "model/Order.h"
#include "repository/IOrderRepository.h"
#include "repository/ISampleRepository.h"

#include <vector>

// PRODUCING 상태 주문을 enqueued_at_millis(승인/생산 진입 시각) 오름차순으로 정렬해서 반환한다.
// ProductionQueueProcessor와 ProductionController가 동일한 FIFO 기준을 공유해야 하므로 공용 함수로 둔다.
std::vector<Order> fetchProducingOrdersByFifo(IOrderRepository& order_repository);

class ProductionQueueProcessor
{
    public:
        ProductionQueueProcessor(ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void advanceQueue();

    private:
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
