#pragma once

#include <cstdint>

class IClock
{
    public:
        virtual ~IClock() = default;

        virtual int64_t nowMillis() = 0;
};
