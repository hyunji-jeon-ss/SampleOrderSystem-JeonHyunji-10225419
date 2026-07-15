#include "controller/OrderController.h"

#include "model/Order.h"
#include "model/OrderStatus.h"
#include "model/Sample.h"

#include <optional>
#include <stdexcept>
#include <string>

OrderController::OrderController(IOrderView& view, IInputReader& input_reader,
    ISampleRepository& sample_repository, IOrderRepository& order_repository)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
    , order_repository(order_repository)
{
}

void OrderController::run()
{
    processReservation();

    view.showMessage("[Enter] 뒤로가기 > ");
    input_reader.readLine();
}

bool OrderController::processReservation()
{
    view.showMessage("시료 ID > ");
    const std::string sample_id = input_reader.readLine();

    const std::optional<Sample> sample = sample_repository.findById(sample_id);
    if (!sample.has_value())
    {
        view.showMessage("등록되지 않은 시료 ID입니다.");
        return false;
    }

    view.showMessage("고객명 > ");
    const std::string customer_name = input_reader.readLine();

    view.showMessage("주문 수량 > ");
    const std::string quantity_text = input_reader.readLine();

    int quantity = 0;
    try
    {
        quantity = std::stoi(quantity_text);
    }
    catch (const std::exception&)
    {
        view.showMessage("수량은 1 이상의 숫자로 입력해주세요.");
        return false;
    }

    if (quantity < 1)
    {
        view.showMessage("수량은 1 이상의 숫자로 입력해주세요.");
        return false;
    }

    view.showConfirmation(*sample, customer_name, quantity);
    const std::string confirm = input_reader.readLine();

    if (confirm != "Y" && confirm != "y")
    {
        view.showMessage("주문이 취소되었습니다.");
        return false;
    }

    const Order saved = order_repository.save(
        Order{ "", sample->id, customer_name, quantity, OrderStatus::RESERVED });
    view.showReservationComplete(saved);
    return true;
}
