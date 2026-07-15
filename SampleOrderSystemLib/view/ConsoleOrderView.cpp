#include "view/ConsoleOrderView.h"

#include "model/OrderStatusText.h"

#include <iostream>

using std::cout;
using std::endl;

void ConsoleOrderView::showConfirmation(const Sample& sample, const std::string& customer_name, int quantity)
{
    cout << endl;
    cout << "입력 내용 확인" << endl;
    cout << "시료   " << sample.name << " (" << sample.id << ")" << endl;
    cout << "고객   " << customer_name << endl;
    cout << "수량   " << quantity << " ea" << endl;
    cout << endl;
    cout << "[Y] 예약 접수   [N] 취소" << endl;
    cout << "선택 > ";
}

void ConsoleOrderView::showReservationComplete(const Order& order)
{
    cout << endl;
    cout << "예약 접수 완료." << endl;
    cout << endl;
    cout << "주문번호   " << order.order_number << endl;
    cout << "현재 상태  " << orderStatusToString(order.status) << endl;
}

void ConsoleOrderView::showMessage(const std::string& message)
{
    cout << message << endl;
}
