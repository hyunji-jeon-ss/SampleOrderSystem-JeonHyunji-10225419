#pragma once

#include "model/OrderStatus.h"

#include <string>

struct Order
{
    std::string order_number;
    std::string sample_id;
    std::string customer_name;
    int quantity = 0;
    OrderStatus status = OrderStatus::RESERVED;
    int shortage_quantity = 0;
    long long enqueued_at_millis = 0;
};
