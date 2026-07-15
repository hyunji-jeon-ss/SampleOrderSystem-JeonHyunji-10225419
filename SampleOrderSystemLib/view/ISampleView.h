#pragma once

#include "model/Sample.h"

#include <string>
#include <vector>

class ISampleView
{
    public:
        virtual ~ISampleView() = default;

        virtual void showSampleMenu() = 0;
        virtual void showSamples(const std::vector<Sample>& samples) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
