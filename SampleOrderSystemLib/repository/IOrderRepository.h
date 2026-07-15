#pragma once

#include "model/Order.h"

#include <optional>
#include <string>
#include <vector>

class IOrderRepository
{
    public:
        virtual ~IOrderRepository() = default;

        virtual Order save(const Order& order) = 0;
        virtual std::optional<Order> findByOrderNumber(const std::string& order_number) = 0;
        virtual std::vector<Order> findAll() = 0;
};
