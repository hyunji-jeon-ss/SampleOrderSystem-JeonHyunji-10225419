#include "controller/SampleController.h"

#include "model/Sample.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace
{
    std::string toLower(const std::string& value)
    {
        std::string lowered = value;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lowered;
    }

    bool containsIgnoreCase(const std::string& text, const std::string& keyword)
    {
        return toLower(text).find(toLower(keyword)) != std::string::npos;
    }
}

SampleController::SampleController(ISampleView& view, IInputReader& input_reader, ISampleRepository& sample_repository)
    : view(view)
    , input_reader(input_reader)
    , sample_repository(sample_repository)
{
}

void SampleController::run()
{
    bool running = true;
    while (running)
    {
        view.showSampleMenu();
        const std::string command = input_reader.readLine();
        running = processCommand(command);
    }
}

bool SampleController::processCommand(const std::string& command)
{
    if (command == "0") return false;

    if (command == "1")
    {
        handleRegister();
        return true;
    }

    if (command == "2")
    {
        handleList();
        return true;
    }

    if (command == "3")
    {
        handleSearch();
        return true;
    }

    view.showMessage("알 수 없는 명령입니다.");
    return true;
}

void SampleController::handleRegister()
{
    view.showMessage("시료 이름 > ");
    const std::string name = input_reader.readLine();

    view.showMessage("평균 생산시간(min, 소수 가능 예: 0.1) > ");
    const std::string time_text = input_reader.readLine();

    view.showMessage("수율(0~1) > ");
    const std::string yield_text = input_reader.readLine();

    double avg_production_time_min = 0.0;
    double yield = 0.0;
    try
    {
        avg_production_time_min = std::stod(time_text);
        yield = std::stod(yield_text);
    }
    catch (const std::exception&)
    {
        view.showMessage("숫자를 올바르게 입력해주세요.");
        return;
    }

    const Sample saved = sample_repository.save(Sample{ "", name, avg_production_time_min, yield, 0, 0 });
    view.showMessage("등록 완료: " + saved.id);
}

void SampleController::handleList()
{
    displayPaged(sample_repository.findAll());
}

void SampleController::handleSearch()
{
    view.showMessage("검색어(이름) > ");
    const std::string keyword = input_reader.readLine();

    std::vector<Sample> matched;
    for (const Sample& sample : sample_repository.findAll())
    {
        if (containsIgnoreCase(sample.name, keyword)) matched.push_back(sample);
    }

    displayPaged(matched);
}

void SampleController::displayPaged(const std::vector<Sample>& samples)
{
    if (samples.empty())
    {
        view.showSamples(samples);
        return;
    }

    constexpr size_t kPageSize = 5;
    size_t offset = 0;
    while (offset < samples.size())
    {
        const size_t end = std::min(offset + kPageSize, samples.size());
        view.showSamples(std::vector<Sample>(samples.begin() + offset, samples.begin() + end));
        offset = end;

        if (offset >= samples.size()) break;

        view.showMessage("[N] 다음 페이지, 그 외 입력 시 종료 > ");
        const std::string command = input_reader.readLine();
        if (command != "N" && command != "n") break;
    }
}
