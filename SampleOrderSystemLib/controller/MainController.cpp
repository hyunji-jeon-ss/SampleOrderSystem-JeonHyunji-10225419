#include "controller/MainController.h"

#include "clock/TimeFormat.h"
#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"

MainController::MainController(IMainView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
    , clock(clock)
{
}

void MainController::run()
{
    bool running = true;
    while (running)
    {
        view.showMainMenu(buildSummary());
        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool MainController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    if (command == "1" || command == "2" || command == "3" ||
        command == "4" || command == "5" || command == "6")
    {
        view.showMessage("아직 구현되지 않은 기능입니다. (다음 Phase에서 구현 예정)");
        return true;
    }

    view.showMessage("알 수 없는 명령입니다.");
    return true;
}

MainMenuSummary MainController::buildSummary()
{
    MainMenuSummary summary;

    summary.current_time_text = formatDateTime(clock.nowMillis());

    const std::vector<Sample> samples = sample_repository.findAll();
    summary.sample_count = static_cast<int>(samples.size());

    // 재고(availableStock/physicalStock) 계산은 Phase 6/8에서 구현 예정. 현재는 0으로 고정.
    summary.total_stock = 0;

    const std::vector<Order> orders = order_repository.findAll();
    summary.order_count = static_cast<int>(orders.size());

    int producing_count = 0;
    for (const Order& order : orders)
    {
        if (order.status == OrderStatus::PRODUCING) producing_count++;
    }
    summary.production_queue_count = producing_count;

    return summary;
}
