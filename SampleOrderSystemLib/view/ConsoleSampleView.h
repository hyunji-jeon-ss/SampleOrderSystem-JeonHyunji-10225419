#pragma once

#include "view/ISampleView.h"

class ConsoleSampleView : public ISampleView
{
    public:
        void showSampleMenu() override;
        void showSamples(const std::vector<Sample>& samples) override;
        void showMessage(const std::string& message) override;
};
