#include "view/ConsoleSampleView.h"

#include <iostream>

using std::cout;
using std::endl;

void ConsoleSampleView::showSampleMenu()
{
    cout << "--------------------------------------------------------------" << endl;
    cout << "[1] 시료 관리" << endl;
    cout << "[1] 시료 등록   [2] 시료 조회   [3] 시료 검색   [0] 뒤로가기" << endl;
    cout << "선택 > ";
}

void ConsoleSampleView::showSamples(const std::vector<Sample>& samples)
{
    if (samples.empty())
    {
        cout << "등록된 시료가 없습니다." << endl;
        return;
    }

    cout << "ID       이름                  평균생산시간(min)   수율    재고" << endl;
    for (const Sample& sample : samples)
    {
        cout << sample.id << "    " << sample.name
            << "   " << sample.avg_production_time_min
            << "   " << sample.yield
            << "   " << sample.physical_stock << " ea" << endl;
    }
}

void ConsoleSampleView::showMessage(const std::string& message)
{
    cout << message << endl;
}
