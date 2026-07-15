#pragma once

#include "model/Order.h"

#include <string>
#include <vector>

struct ApprovalOrderRow
{
    Order order;
    std::string sample_name;
};

class IApprovalView
{
    public:
        virtual ~IApprovalView() = default;

        virtual void showReservedList(const std::vector<ApprovalOrderRow>& rows) = 0;
        virtual void showSufficientStockCheck(const ApprovalOrderRow& row, int available_stock) = 0;
        virtual void showInsufficientStockCheck(const ApprovalOrderRow& row, int available_stock,
            int shortage_quantity, int real_production_quantity, double total_production_time_min) = 0;
        virtual void showApprovalResult(const Order& order) = 0;
        virtual void showRejection(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
