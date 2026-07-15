#include "controller/ReleaseController.h"

#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"

#include <optional>
#include <stdexcept>

ReleaseController::ReleaseController(IReleaseView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
{
}

void ReleaseController::run()
{
    bool running = true;
    while (running)
    {
        const std::vector<ReleaseOrderRow> rows = buildConfirmedRows();
        view.showConfirmedList(rows);
        if (rows.empty())
        {
            view.showMessage("[0] 뒤로가기 > ");
        }

        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool ReleaseController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    const std::vector<ReleaseOrderRow> rows = buildConfirmedRows();

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

    handleReleaseFlow(rows[index - 1]);
    return true;
}

std::vector<ReleaseOrderRow> ReleaseController::buildConfirmedRows()
{
    std::vector<ReleaseOrderRow> rows;
    for (const Order& order : order_repository.findAll())
    {
        if (order.status != OrderStatus::CONFIRMED) continue;

        std::string sample_name = order.sample_id;
        const std::optional<Sample> sample = sample_repository.findById(order.sample_id);
        if (sample.has_value()) sample_name = sample->name;

        rows.push_back(ReleaseOrderRow{ order, sample_name });
    }
    return rows;
}

void ReleaseController::handleReleaseFlow(const ReleaseOrderRow& row)
{
    const std::optional<Sample> found_sample = sample_repository.findById(row.order.sample_id);
    if (!found_sample.has_value())
    {
        view.showMessage("시료 정보를 찾을 수 없습니다.");
        return;
    }

    Sample sample = *found_sample;
    view.showReleaseCheck(row, sample.physical_stock);

    const std::string confirm = input_reader.readLine();
    if (confirm != "Y" && confirm != "y")
    {
        view.showCancellation(row.order);
        return;
    }

    sample.physical_stock -= row.order.quantity;
    sample_repository.save(sample);

    Order updated = row.order;
    updated.status = OrderStatus::RELEASED;
    order_repository.save(updated);

    view.showReleaseResult(updated);
}
