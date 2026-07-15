#pragma once

#include "view/IApprovalView.h"

class ConsoleApprovalView : public IApprovalView
{
    public:
        void showReservedList(const std::vector<ApprovalOrderRow>& rows) override;
        void showSufficientStockCheck(const ApprovalOrderRow& row, int available_stock) override;
        void showInsufficientStockCheck(const ApprovalOrderRow& row, int available_stock,
            int shortage_quantity, int real_production_quantity, double total_production_time_min) override;
        void showApprovalResult(const Order& order) override;
        void showRejection(const Order& order) override;
        void showMessage(const std::string& message) override;
};
