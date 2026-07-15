#include "view/ConsoleMainView.h"

#include <iostream>

using std::cout;
using std::endl;

void ConsoleMainView::showMainMenu(const MainMenuSummary& summary)
{
    cout << "==============================================================" << endl;
    cout << "  반도체 시료 생산주문관리 시스템" << endl;
    cout << "==============================================================" << endl;
    cout << "시스템 현황   " << summary.current_time_text << endl;
    cout << "등록 시료 " << summary.sample_count << "종   총 재고 " << summary.total_stock << " ea" << endl;
    cout << "전체 주문 " << summary.order_count << "건   생산라인 " << summary.production_queue_count << "건 대기" << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << "[1] 시료 관리       [2] 시료 주문" << endl;
    cout << "[3] 주문 승인/거절  [4] 모니터링" << endl;
    cout << "[5] 생산라인 조회   [6] 출고 처리" << endl;
    cout << "[0] 종료" << endl;
    cout << "선택 > ";
}

void ConsoleMainView::showMessage(const std::string& message)
{
    cout << message << endl;
}
