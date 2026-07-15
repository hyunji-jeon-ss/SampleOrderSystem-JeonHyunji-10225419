#include "clock/SystemClock.h"
#include "controller/ApprovalController.h"
#include "controller/ConsoleInputReader.h"
#include "controller/MainController.h"
#include "controller/MonitoringController.h"
#include "controller/OrderController.h"
#include "controller/ProductionController.h"
#include "controller/ReleaseController.h"
#include "controller/SampleController.h"
#include "production/ProductionQueueProcessor.h"
#include "repository/JsonOrderRepository.h"
#include "repository/JsonSampleRepository.h"
#include "view/ConsoleApprovalView.h"
#include "view/ConsoleMainView.h"
#include "view/ConsoleMonitoringView.h"
#include "view/ConsoleOrderView.h"
#include "view/ConsoleProductionView.h"
#include "view/ConsoleReleaseView.h"
#include "view/ConsoleSampleView.h"

#define NOMINMAX
#include <windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SystemClock clock;
    JsonSampleRepository sample_repository("samples.json");
    JsonOrderRepository order_repository("orders.json", clock);

    ConsoleMainView view;
    ConsoleSampleView sample_view;
    ConsoleOrderView order_view;
    ConsoleApprovalView approval_view;
    ConsoleProductionView production_view;
    ConsoleReleaseView release_view;
    ConsoleMonitoringView monitoring_view;
    ConsoleInputReader input_reader;

    ProductionQueueProcessor queue_processor(sample_repository, order_repository, clock);

    SampleController sample_menu(sample_view, input_reader, sample_repository);
    OrderController order_menu(order_view, input_reader, sample_repository, order_repository);
    ApprovalController approval_menu(approval_view, input_reader, sample_repository, order_repository, clock);
    ProductionController production_menu(
        production_view, input_reader, sample_repository, order_repository, queue_processor, clock);
    ReleaseController release_menu(release_view, input_reader, sample_repository, order_repository);
    MonitoringController monitoring_menu(
        monitoring_view, input_reader, sample_repository, order_repository, clock);
    MainController controller(view, input_reader, sample_repository, order_repository, clock,
        &sample_menu, &order_menu, &approval_menu, &production_menu, &queue_processor, &release_menu,
        &monitoring_menu);
    controller.run();

    return 0;
}
