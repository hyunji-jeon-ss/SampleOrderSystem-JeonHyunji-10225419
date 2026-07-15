#include "controller/MainController.h"

#include "clock/TimeFormat.h"
#include "console/ConsoleUtil.h"
#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"

MainController::MainController(IMainView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock,
    ISubMenuController* sample_menu, ISubMenuController* order_menu, ISubMenuController* approval_menu,
    ISubMenuController* production_menu, ProductionQueueProcessor* production_queue_processor,
    ISubMenuController* release_menu)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
    , clock(clock)
    , sample_menu(sample_menu)
    , order_menu(order_menu)
    , approval_menu(approval_menu)
    , production_menu(production_menu)
    , production_queue_processor(production_queue_processor)
    , release_menu(release_menu)
{
}

void MainController::run()
{
    bool running = true;
    while (running)
    {
        if (production_queue_processor) production_queue_processor->advanceQueue();

        clearConsoleScreen();
        view.showMainMenu(buildSummary());
        const std::string command = input_reader.readLine();
        clearConsoleScreen();
        running = processCommand(command);
    }
}

bool MainController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    if (command == "1")
    {
        runSubMenuOrShowPlaceholder(sample_menu);
        return true;
    }

    if (command == "2")
    {
        runSubMenuOrShowPlaceholder(order_menu);
        return true;
    }

    if (command == "3")
    {
        runSubMenuOrShowPlaceholder(approval_menu);
        return true;
    }

    if (command == "5")
    {
        runSubMenuOrShowPlaceholder(production_menu);
        return true;
    }

    if (command == "6")
    {
        runSubMenuOrShowPlaceholder(release_menu);
        return true;
    }

    if (command == "4")
    {
        runSubMenuOrShowPlaceholder(nullptr);
        return true;
    }

    view.showMessage("알 수 없는 명령입니다.");
    return true;
}

void MainController::runSubMenuOrShowPlaceholder(ISubMenuController* menu)
{
    if (menu)
    {
        menu->run();
    }
    else
    {
        view.showMessage("아직 구현되지 않은 기능입니다. (다음 Phase에서 구현 예정)");
    }
}

MainMenuSummary MainController::buildSummary()
{
    MainMenuSummary summary;

    summary.current_time_text = formatDateTime(clock.nowMillis());

    const std::vector<Sample> samples = sample_repository.findAll();
    summary.sample_count = static_cast<int>(samples.size());

    int total_stock = 0;
    for (const Sample& sample : samples)
    {
        total_stock += sample.physical_stock;
    }
    summary.total_stock = total_stock;

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
