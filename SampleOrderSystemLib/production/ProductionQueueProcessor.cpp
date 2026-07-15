#include "production/ProductionQueueProcessor.h"

#include "model/OrderStatus.h"
#include "model/Sample.h"
#include "production/ProductionCalculator.h"

#include <algorithm>
#include <optional>

ProductionQueueProcessor::ProductionQueueProcessor(
    ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock)
    : sample_repository(sample_repository)
    , order_repository(order_repository)
    , clock(clock)
{
}

std::vector<Order> fetchProducingOrdersByFifo(IOrderRepository& order_repository)
{
    std::vector<Order> producing;
    for (const Order& order : order_repository.findAll())
    {
        if (order.status == OrderStatus::PRODUCING) producing.push_back(order);
    }

    std::sort(producing.begin(), producing.end(),
        [](const Order& a, const Order& b) { return a.enqueued_at_millis < b.enqueued_at_millis; });

    return producing;
}

void ProductionQueueProcessor::advanceQueue()
{
    long long last_completion_millis = -1;

    while (true)
    {
        const std::vector<Order> producing = fetchProducingOrdersByFifo(order_repository);
        if (producing.empty()) return;

        Order active = producing.front();

        if (active.production_start_millis == 0)
        {
            const std::optional<Sample> sample = sample_repository.findById(active.sample_id);
            if (!sample.has_value()) return;

            const int real_qty = computeRealProductionQuantity(active.shortage_quantity, sample->yield);
            const double total_min = computeTotalProductionTimeMin(real_qty, sample->avg_production_time_min);

            const long long start = (last_completion_millis >= 0) ? last_completion_millis : clock.nowMillis();

            active.real_production_quantity = real_qty;
            active.production_start_millis = start;
            active.production_end_millis = start + static_cast<long long>(total_min * 60000.0);
            order_repository.save(active);

            if (active.production_end_millis > clock.nowMillis()) return;
        }

        if (clock.nowMillis() < active.production_end_millis) return;

        const std::optional<Sample> sample = sample_repository.findById(active.sample_id);
        if (sample.has_value())
        {
            Sample updated_sample = *sample;
            updated_sample.physical_stock += active.real_production_quantity;
            sample_repository.save(updated_sample);
        }

        active.status = OrderStatus::CONFIRMED;
        order_repository.save(active);

        last_completion_millis = active.production_end_millis;
    }
}
