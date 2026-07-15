#include "controller/ConsoleInputReader.h"

#include <iostream>
#include <string>

std::string ConsoleInputReader::readLine()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}
