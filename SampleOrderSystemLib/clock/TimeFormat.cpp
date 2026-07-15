#include "clock/TimeFormat.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace
{
    std::tm toLocalTime(int64_t millis)
    {
        const std::time_t seconds = static_cast<std::time_t>(millis / 1000);
        std::tm local_time{};
        localtime_s(&local_time, &seconds);
        return local_time;
    }
}

std::string formatDate(int64_t millis)
{
    const std::tm local_time = toLocalTime(millis);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y%m%d");
    return oss.str();
}

std::string formatDateTime(int64_t millis)
{
    const std::tm local_time = toLocalTime(millis);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string formatShortDateTime(int64_t millis)
{
    const std::tm local_time = toLocalTime(millis);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%y/%m/%d %H:%M");
    return oss.str();
}
