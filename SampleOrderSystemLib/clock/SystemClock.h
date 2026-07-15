#pragma once

#include "clock/IClock.h"

class SystemClock : public IClock
{
    public:
        int64_t nowMillis() override;
};
