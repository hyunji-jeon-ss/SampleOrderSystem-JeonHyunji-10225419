#pragma once

#include "view/IReleaseView.h"

class ConsoleReleaseView : public IReleaseView
{
    public:
        void showConfirmedList(const std::vector<ReleaseOrderRow>& rows) override;
        void showReleaseCheck(const ReleaseOrderRow& row, int physical_stock) override;
        void showReleaseResult(const Order& order) override;
        void showCancellation(const Order& order) override;
        void showMessage(const std::string& message) override;
};
