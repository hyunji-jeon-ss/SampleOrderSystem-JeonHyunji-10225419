#include "controller/ProductionController.h"

#include "clock/TimeFormat.h"
#include "console/ConsoleUtil.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"
#include "production/ProductionCalculator.h"

#include <algorithm>
#include <optional>

ProductionController::ProductionController(IProductionView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository,
    ProductionQueueProcessor& queue_processor, IClock& clock)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
    , queue_processor(queue_processor)
    , clock(clock)
{
}

void ProductionController::run()
{
    bool running = true;
    while (running)
    {
        clearConsoleScreen();
        display();
        view.showMessage("[R] 새로고침   [0] 뒤로가기 > ");

        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool ProductionController::processCommand(const std::string& command)
{
    if (command == "0") return false;
    return true;
}

void ProductionController::display()
{
    queue_processor.advanceQueue();

    const std::vector<Order> producing = fetchProducingOrdersByFifo(order_repository);

    view.showLineStatus(!producing.empty(), formatDateTime(clock.nowMillis()));

    long long cursor_millis = clock.nowMillis();

    if (producing.empty())
    {
        view.showActiveProduction(std::nullopt);
    }
    else
    {
        const Order& active_order = producing.front();
        std::string sample_name = active_order.sample_id;
        double yield = 1.0;
        double total_production_time_min = 0.0;

        const std::optional<Sample> sample = sample_repository.findById(active_order.sample_id);
        if (sample.has_value())
        {
            sample_name = sample->name;
            yield = sample->yield;
            total_production_time_min =
                computeTotalProductionTimeMin(active_order.real_production_quantity, sample->avg_production_time_min);
        }

        const long long start = active_order.production_start_millis;
        const long long end = active_order.production_end_millis;
        int progress_percent = 100;
        if (end > start)
        {
            const long long now = clock.nowMillis();
            progress_percent = static_cast<int>(std::clamp<long long>((now - start) * 100 / (end - start), 0, 100));
        }

        ActiveProductionInfo info;
        info.order = active_order;
        info.sample_name = sample_name;
        info.available_stock_at_approval = active_order.quantity - active_order.shortage_quantity;
        info.yield = yield;
        info.total_production_time_min = total_production_time_min;
        info.progress_percent = progress_percent;

        view.showActiveProduction(info);

        cursor_millis = end;
    }

    std::vector<QueuedProductionInfo> queue;
    for (std::size_t i = 1; i < producing.size(); i++)
    {
        const Order& order = producing[i];
        std::string sample_name = order.sample_id;
        int projected_real_production_quantity = 0;
        double total_production_time_min = 0.0;

        const std::optional<Sample> sample = sample_repository.findById(order.sample_id);
        if (sample.has_value())
        {
            sample_name = sample->name;
            projected_real_production_quantity = computeRealProductionQuantity(order.shortage_quantity, sample->yield);
            total_production_time_min =
                computeTotalProductionTimeMin(projected_real_production_quantity, sample->avg_production_time_min);
        }

        cursor_millis += static_cast<long long>(total_production_time_min * 60000.0);

        QueuedProductionInfo info;
        info.order = order;
        info.sample_name = sample_name;
        info.projected_real_production_quantity = projected_real_production_quantity;
        info.expected_completion_millis = cursor_millis;

        queue.push_back(info);
    }

    view.showQueue(queue);
}
