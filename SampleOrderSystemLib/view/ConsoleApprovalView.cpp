#include "view/ConsoleApprovalView.h"

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

void ConsoleApprovalView::showReservedList(const std::vector<ApprovalOrderRow>& rows)
{
    if (rows.empty())
    {
        cout << "승인 대기 중인 주문이 없습니다." << endl;
        return;
    }

    cout << padEnd("번호", kIndexWidth) << padEnd("주문번호", kOrderNumberWidth)
        << padEnd("고객", kCustomerWidth) << padEnd("시료", kSampleWidth)
        << padStart("수량", kQuantityWidth) << endl;

    for (std::size_t i = 0; i < rows.size(); i++)
    {
        const ApprovalOrderRow& row = rows[i];
        cout << padEnd(std::to_string(i + 1), kIndexWidth) << padEnd(row.order.order_number, kOrderNumberWidth)
            << padEnd(row.order.customer_name, kCustomerWidth) << padEnd(row.sample_name, kSampleWidth)
            << padStart(std::to_string(row.order.quantity) + " ea", kQuantityWidth) << endl;
    }

    cout << "선택 > ";
}

void ConsoleApprovalView::showSufficientStockCheck(const ApprovalOrderRow& row, int available_stock)
{
    cout << endl;
    cout << "시료          " << row.sample_name << " (" << row.order.sample_id << ")" << endl;
    cout << "현재 재고(가용)  " << available_stock << " ea" << endl;
    cout << "주문 수량        " << row.order.quantity << " ea" << endl;
    cout << endl;
    cout << "재고가 충분합니다. 승인하시겠습니까?" << endl;
    cout << "[Y] 승인   [N] 거절" << endl;
    cout << "선택 > ";
}

void ConsoleApprovalView::showInsufficientStockCheck(const ApprovalOrderRow& row, int available_stock,
    int shortage_quantity, int real_production_quantity, double total_production_time_min)
{
    cout << endl;
    cout << "시료          " << row.sample_name << " (" << row.order.sample_id << ")" << endl;
    cout << "현재 재고(가용)  " << available_stock << " ea" << endl;
    cout << "주문 수량        " << row.order.quantity << " ea" << endl;
    cout << "부족분           " << shortage_quantity << " ea" << endl;
    cout << endl;
    cout << "재고 부족. 실생산량 " << real_production_quantity << " ea (예상 " << total_production_time_min
        << " min) 생산으로 등록하고 승인하시겠습니까?" << endl;
    cout << "[Y] 승인   [N] 거절" << endl;
    cout << "선택 > ";
}

void ConsoleApprovalView::showApprovalResult(const Order& order)
{
    cout << endl;
    cout << "승인 완료." << endl;
    cout << "주문번호   " << order.order_number << endl;
    cout << "현재 상태  " << orderStatusToString(order.status) << endl;
}

void ConsoleApprovalView::showRejection(const Order& order)
{
    cout << endl;
    cout << "주문을 거절했습니다." << endl;
    cout << "주문번호   " << order.order_number << endl;
    cout << "현재 상태  " << orderStatusToString(order.status) << endl;
}

void ConsoleApprovalView::showMessage(const std::string& message)
{
    cout << message << endl;
}
