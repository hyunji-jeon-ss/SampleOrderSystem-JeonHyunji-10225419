#include "view/ConsoleMonitoringView.h"

#include "console/ConsoleUtil.h"

#include <iostream>

using std::cout;
using std::endl;

namespace
{
    constexpr std::size_t kIdWidth = 8;
    constexpr std::size_t kNameWidth = 26;
    constexpr std::size_t kStockWidth = 12;

    std::string toText(StockStatus status)
    {
        switch (status)
        {
            case StockStatus::SUFFICIENT: return "여유";
            case StockStatus::SHORTAGE: return "부족";
            case StockStatus::DEPLETED: return "고갈";
        }
        return "여유";
    }
}

void ConsoleMonitoringView::showMonitoring(const std::string& current_time_text,
    const OrderStatusSummary& order_summary, const std::vector<StockStatusRow>& stock_rows)
{
    cout << "--------------------------------------------------------------" << endl;
    cout << "[4] 모니터링   현재시각 " << current_time_text << endl;
    cout << endl;
    cout << "주문 현황" << endl;
    cout << "  예약중(RESERVED)     " << order_summary.reserved_count << "건" << endl;
    cout << "  승인완료(CONFIRMED)  " << order_summary.confirmed_count << "건" << endl;
    cout << "  생산중(PRODUCING)    " << order_summary.producing_count << "건" << endl;
    cout << "  출고완료(RELEASED)   " << order_summary.released_count << "건" << endl;
    cout << endl;
    cout << "재고 현황" << endl;

    if (stock_rows.empty())
    {
        cout << "등록된 시료가 없습니다." << endl;
    }
    else
    {
        cout << padEnd("ID", kIdWidth) << padEnd("이름", kNameWidth)
            << padStart("재고", kStockWidth) << "      상태" << endl;

        for (const StockStatusRow& row : stock_rows)
        {
            cout << padEnd(row.sample_id, kIdWidth) << padEnd(row.sample_name, kNameWidth)
                << padStart(std::to_string(row.physical_stock) + " ea", kStockWidth)
                << "      " << toText(row.status) << endl;
        }
    }

    cout << endl;
}

void ConsoleMonitoringView::showMessage(const std::string& message)
{
    cout << message << endl;
}
