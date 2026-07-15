#include "view/ConsoleSampleView.h"

#include "console/ConsoleUtil.h"

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;

namespace
{
    constexpr std::size_t kIdWidth = 10;
    constexpr std::size_t kNameWidth = 24;
    constexpr std::size_t kTimeWidth = 16;
    constexpr std::size_t kYieldWidth = 8;
    constexpr std::size_t kStockWidth = 10;

    std::string toText(double value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
}

void ConsoleSampleView::showSampleMenu()
{
    cout << "--------------------------------------------------------------" << endl;
    cout << "시료 관리" << endl;
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

    cout << padEnd("ID", kIdWidth) << padEnd("이름", kNameWidth)
        << padStart("생산시간(min)", kTimeWidth)
        << padStart("수율", kYieldWidth)
        << padStart("재고", kStockWidth) << endl;

    for (const Sample& sample : samples)
    {
        cout << padEnd(sample.id, kIdWidth) << padEnd(sample.name, kNameWidth)
            << padStart(toText(sample.avg_production_time_min), kTimeWidth)
            << padStart(toText(sample.yield), kYieldWidth)
            << padStart(std::to_string(sample.physical_stock) + " ea", kStockWidth) << endl;
    }
}

void ConsoleSampleView::showMessage(const std::string& message)
{
    cout << message << endl;
}
