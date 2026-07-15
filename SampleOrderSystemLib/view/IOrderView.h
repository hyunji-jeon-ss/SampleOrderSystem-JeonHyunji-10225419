#pragma once

#include "model/Order.h"
#include "model/Sample.h"

#include <string>

class IOrderView
{
    public:
        virtual ~IOrderView() = default;

        virtual void showConfirmation(const Sample& sample, const std::string& customer_name, int quantity) = 0;
        virtual void showReservationComplete(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
