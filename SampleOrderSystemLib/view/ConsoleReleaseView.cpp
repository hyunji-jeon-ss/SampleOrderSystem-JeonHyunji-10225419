#include "view/ConsoleReleaseView.h"

#include "console/ConsoleUtil.h"
#include "model/OrderStatusText.h"

#include <iostream>
#include <string>

using std::cout;
using std::endl;

namespace
{
    constexpr std::size_t kIndexWidth = 6;
    constexpr std::size_t kOrderNumberWidth = 20;
    constexpr std::size_t kCustomerWidth = 20;
    constexpr std::size_t kSampleWidth = 22;
    constexpr std::size_t kQuantityWidth = 10;
}

void ConsoleReleaseView::showConfirmedList(const std::vector<ReleaseOrderRow>& rows)
{
    if (rows.empty())
    {
        cout << "출고 대기 중인 주문이 없습니다." << endl;
        return;
    }

    cout << padEnd("번호", kIndexWidth) << padEnd("주문번호", kOrderNumberWidth)
        << padEnd("고객", kCustomerWidth) << padEnd("시료", kSampleWidth)
        << padStart("수량", kQuantityWidth) << endl;

    for (std::size_t i = 0; i < rows.size(); i++)
    {
        const ReleaseOrderRow& row = rows[i];
        cout << padEnd(std::to_string(i + 1), kIndexWidth) << padEnd(row.order.order_number, kOrderNumberWidth)
            << padEnd(row.order.customer_name, kCustomerWidth) << padEnd(row.sample_name, kSampleWidth)
            << padStart(std::to_string(row.order.quantity) + " ea", kQuantityWidth) << endl;
    }

    cout << "선택 > ";
}

void ConsoleReleaseView::showReleaseCheck(const ReleaseOrderRow& row, int physical_stock)
{
    cout << endl;
    cout << "시료          " << row.sample_name << " (" << row.order.sample_id << ")" << endl;
    cout << "현재 재고(화면)  " << physical_stock << " ea" << endl;
    cout << "출고 수량        " << row.order.quantity << " ea" << endl;
    cout << endl;
    cout << "출고 처리하시겠습니까?" << endl;
    cout << "[Y] 출고   [N] 취소" << endl;
    cout << "선택 > ";
}

void ConsoleReleaseView::showReleaseResult(const Order& order)
{
    cout << endl;
    cout << "출고 완료." << endl;
    cout << "주문번호   " << order.order_number << endl;
    cout << "현재 상태  " << orderStatusToString(order.status) << endl;
}

void ConsoleReleaseView::showCancellation(const Order& order)
{
    cout << endl;
    cout << "출고를 취소했습니다." << endl;
    cout << "주문번호   " << order.order_number << endl;
    cout << "현재 상태  " << orderStatusToString(order.status) << endl;
}

void ConsoleReleaseView::showMessage(const std::string& message)
{
    cout << message << endl;
}
