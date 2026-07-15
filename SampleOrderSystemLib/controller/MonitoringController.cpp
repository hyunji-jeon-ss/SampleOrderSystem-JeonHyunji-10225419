#include "controller/MonitoringController.h"

#include "clock/TimeFormat.h"
#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"

MonitoringController::MonitoringController(IMonitoringView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
    , clock(clock)
{
}

void MonitoringController::run()
{
    bool running = true;
    while (running)
    {
        view.showMenu(formatDateTime(clock.nowMillis()));

        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool MonitoringController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    if (command == "1")
    {
        view.showOrderStatus(buildOrderStatusSummary());
        return true;
    }

    if (command == "2")
    {
        view.showStockStatus(buildStockStatusRows());
        return true;
    }

    view.showMessage("알 수 없는 명령입니다.");
    return true;
}

OrderStatusSummary MonitoringController::buildOrderStatusSummary()
{
    OrderStatusSummary summary;

    for (const Order& order : order_repository.findAll())
    {
        switch (order.status)
        {
            case OrderStatus::RESERVED: summary.reserved_count++; break;
            case OrderStatus::CONFIRMED: summary.confirmed_count++; break;
            case OrderStatus::PRODUCING: summary.producing_count++; break;
            case OrderStatus::RELEASED: summary.released_count++; break;
            case OrderStatus::REJECTED: break;
        }
    }

    return summary;
}

std::vector<StockStatusRow> MonitoringController::buildStockStatusRows()
{
    const std::vector<Order> orders = order_repository.findAll();

    std::vector<StockStatusRow> rows;
    for (const Sample& sample : sample_repository.findAll())
    {
        int pending_demand = 0;
        for (const Order& order : orders)
        {
            if (order.sample_id != sample.id) continue;
            if (order.status != OrderStatus::RESERVED && order.status != OrderStatus::PRODUCING
                && order.status != OrderStatus::CONFIRMED) continue;

            pending_demand += order.quantity;
        }

        StockStatus status = StockStatus::SUFFICIENT;
        if (sample.physical_stock == 0) status = StockStatus::DEPLETED;
        else if (sample.physical_stock < pending_demand) status = StockStatus::SHORTAGE;

        rows.push_back(StockStatusRow{ sample.id, sample.name, sample.physical_stock, pending_demand, status });
    }

    return rows;
}
