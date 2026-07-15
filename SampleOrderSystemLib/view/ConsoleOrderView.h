#pragma once

#include "view/IOrderView.h"

class ConsoleOrderView : public IOrderView
{
    public:
        void showConfirmation(const Sample& sample, const std::string& customer_name, int quantity) override;
        void showReservationComplete(const Order& order) override;
        void showMessage(const std::string& message) override;
};
