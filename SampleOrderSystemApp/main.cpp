#include "clock/SystemClock.h"
#include "controller/ConsoleInputReader.h"
#include "controller/MainController.h"
#include "controller/SampleController.h"
#include "repository/JsonOrderRepository.h"
#include "repository/JsonSampleRepository.h"
#include "view/ConsoleMainView.h"
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
    ConsoleInputReader input_reader;

    SampleController sample_menu(sample_view, input_reader, sample_repository);
    MainController controller(view, input_reader, sample_repository, order_repository, clock, &sample_menu);
    controller.run();

    return 0;
}
