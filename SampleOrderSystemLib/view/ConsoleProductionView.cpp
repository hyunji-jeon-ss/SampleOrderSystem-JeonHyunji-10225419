#include "view/ConsoleProductionView.h"

#include "clock/TimeFormat.h"
#include "console/ConsoleUtil.h"

#include <iostream>

using std::cout;
using std::endl;

namespace
{
    constexpr std::size_t kIndexWidth = 6;
    constexpr std::size_t kOrderNumberWidth = 20;
    constexpr std::size_t kSampleWidth = 22;
    constexpr std::size_t kQuantityWidth = 10;
    constexpr int kProgressBarWidth = 20;
}

void ConsoleProductionView::showLineStatus(bool is_running)
{
    cout << "--------------------------------------------------------------" << endl;
    cout << "[5] 생산라인 조회   FIFO 방식" << endl;
    cout << "생산라인 1개 (단일 라인)   현재 상태: " << (is_running ? "RUNNING" : "IDLE") << endl;
    cout << endl;
}

void ConsoleProductionView::showActiveProduction(const std::optional<ActiveProductionInfo>& active)
{
    if (!active.has_value())
    {
        cout << "현재 생산 중인 항목이 없습니다." << endl;
        return;
    }

    const ActiveProductionInfo& info = *active;

    cout << "현재 처리 중" << endl;
    cout << "  주문번호   " << info.order.order_number << "   시료   " << info.sample_name << endl;
    cout << "  주문량   " << info.order.quantity << " ea   재고   " << info.available_stock_at_approval
        << " ea -> 부족 " << info.order.shortage_quantity << " ea -> 실생산량 " << info.order.real_production_quantity
        << " ea (수율 " << info.yield << " / 총 진행시간 " << info.total_production_time_min << " min)" << endl;
    cout << "  진행   " << renderProgressBar(info.progress_percent, kProgressBarWidth) << "  " << info.progress_percent
        << "%   완료 예정 " << formatDateTime(info.order.production_end_millis) << endl;
}

void ConsoleProductionView::showQueue(const std::vector<QueuedProductionInfo>& queue)
{
    cout << endl;
    cout << "대기 중인 주문 (FIFO 순)" << endl;

    if (queue.empty())
    {
        cout << "대기 중인 주문이 없습니다." << endl;
        return;
    }

    cout << padEnd("순서", kIndexWidth) << padEnd("주문번호", kOrderNumberWidth)
        << padEnd("시료", kSampleWidth) << padStart("주문량", kQuantityWidth)
        << padStart("부족분", kQuantityWidth) << padStart("실생산량", kQuantityWidth)
        << padStart("예상완료", kQuantityWidth) << endl;

    for (std::size_t i = 0; i < queue.size(); i++)
    {
        const QueuedProductionInfo& info = queue[i];
        cout << padEnd(std::to_string(i + 1), kIndexWidth) << padEnd(info.order.order_number, kOrderNumberWidth)
            << padEnd(info.sample_name, kSampleWidth)
            << padStart(std::to_string(info.order.quantity) + " ea", kQuantityWidth)
            << padStart(std::to_string(info.order.shortage_quantity) + " ea", kQuantityWidth)
            << padStart(std::to_string(info.projected_real_production_quantity) + " ea", kQuantityWidth)
            << padStart(formatDateTime(info.expected_completion_millis), kQuantityWidth) << endl;
    }
}

void ConsoleProductionView::showMessage(const std::string& message)
{
    cout << message << endl;
}
