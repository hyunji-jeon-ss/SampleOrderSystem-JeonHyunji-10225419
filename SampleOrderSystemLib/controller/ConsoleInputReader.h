#pragma once

#include "controller/IInputReader.h"

class ConsoleInputReader : public IInputReader
{
    public:
        std::string readLine() override;
};
