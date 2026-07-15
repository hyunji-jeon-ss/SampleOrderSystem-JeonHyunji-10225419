#include "controller/ApprovalController.h"

#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"
#include "production/ProductionCalculator.h"

#include <optional>
#include <stdexcept>

ApprovalController::ApprovalController(IApprovalView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
    , clock(clock)
{
}

void ApprovalController::run()
{
    bool running = true;
    while (running)
    {
        const std::vector<ApprovalOrderRow> rows = buildReservedRows();
        view.showReservedList(rows);
        if (rows.empty())
        {
            view.showMessage("[0] 뒤로가기 > ");
        }

        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool ApprovalController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    const std::vector<ApprovalOrderRow> rows = buildReservedRows();

    int index = 0;
    try
    {
        index = std::stoi(command);
    }
    catch (const std::exception&)
    {
        view.showMessage("알 수 없는 명령입니다.");
        return true;
    }

    if (index < 1 || static_cast<std::size_t>(index) > rows.size())
    {
        view.showMessage("잘못된 번호입니다.");
        return true;
    }

    handleApprovalFlow(rows[index - 1]);
    return true;
}

std::vector<ApprovalOrderRow> ApprovalController::buildReservedRows()
{
    std::vector<ApprovalOrderRow> rows;
    for (const Order& order : order_repository.findAll())
    {
        if (order.status != OrderStatus::RESERVED) continue;

        std::string sample_name = order.sample_id;
        const std::optional<Sample> sample = sample_repository.findById(order.sample_id);
        if (sample.has_value()) sample_name = sample->name;

        rows.push_back(ApprovalOrderRow{ order, sample_name });
    }
    return rows;
}

void ApprovalController::handleApprovalFlow(const ApprovalOrderRow& row)
{
    const std::optional<Sample> found_sample = sample_repository.findById(row.order.sample_id);
    if (!found_sample.has_value())
    {
        view.showMessage("시료 정보를 찾을 수 없습니다.");
        return;
    }

    Sample sample = *found_sample;
    const bool sufficient = sample.available_stock >= row.order.quantity;

    if (sufficient)
    {
        view.showSufficientStockCheck(row, sample.available_stock);
    }
    else
    {
        const int shortage = row.order.quantity - sample.available_stock;
        const int real_qty = computeRealProductionQuantity(shortage, sample.yield);
        const double total_min = computeTotalProductionTimeMin(real_qty, sample.avg_production_time_min);
        view.showInsufficientStockCheck(row, sample.available_stock, shortage, real_qty, total_min);
    }

    const std::string confirm = input_reader.readLine();
    if (confirm != "Y" && confirm != "y")
    {
        Order rejected = row.order;
        rejected.status = OrderStatus::REJECTED;
        order_repository.save(rejected);
        view.showRejection(rejected);
        return;
    }

    Order updated = row.order;
    if (sufficient)
    {
        sample.available_stock -= row.order.quantity;
        updated.status = OrderStatus::CONFIRMED;
    }
    else
    {
        const int shortage = row.order.quantity - sample.available_stock;
        sample.available_stock = 0;
        updated.status = OrderStatus::PRODUCING;
        updated.shortage_quantity = shortage;
        updated.enqueued_at_millis = clock.nowMillis();
    }

    sample_repository.save(sample);
    order_repository.save(updated);
    view.showApprovalResult(updated);
}
