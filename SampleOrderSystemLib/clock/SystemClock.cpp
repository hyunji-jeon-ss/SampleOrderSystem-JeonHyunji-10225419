#include "clock/SystemClock.h"

#include <chrono>

int64_t SystemClock::nowMillis()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
