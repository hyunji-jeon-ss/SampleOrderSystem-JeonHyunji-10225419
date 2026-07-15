#pragma once

#include "model/Order.h"

#include <string>
#include <vector>

struct ReleaseOrderRow
{
    Order order;
    std::string sample_name;
};

class IReleaseView
{
    public:
        virtual ~IReleaseView() = default;

        virtual void showConfirmedList(const std::vector<ReleaseOrderRow>& rows) = 0;
        virtual void showReleaseCheck(const ReleaseOrderRow& row, int physical_stock) = 0;
        virtual void showReleaseResult(const Order& order) = 0;
        virtual void showCancellation(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
