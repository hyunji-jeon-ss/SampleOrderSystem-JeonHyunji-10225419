#include "clock/SystemClock.h"
#include "controller/ConsoleInputReader.h"
#include "controller/MainController.h"
#include "repository/JsonOrderRepository.h"
#include "repository/JsonSampleRepository.h"
#include "view/ConsoleMainView.h"

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
    ConsoleInputReader input_reader;

    MainController controller(view, input_reader, sample_repository, order_repository, clock);
    controller.run();

    return 0;
}
