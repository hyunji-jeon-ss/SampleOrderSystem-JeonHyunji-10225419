#pragma once

#include <string>

class IInputReader
{
    public:
        virtual ~IInputReader() = default;

        virtual std::string readLine() = 0;
};
