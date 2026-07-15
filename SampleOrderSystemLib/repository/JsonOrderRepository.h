#pragma once

#include "clock/IClock.h"
#include "repository/IOrderRepository.h"

#include <string>

class JsonOrderRepository : public IOrderRepository
{
    public:
        JsonOrderRepository(const std::string& file_path, IClock& clock);

        Order save(const Order& order) override;
        std::optional<Order> findByOrderNumber(const std::string& order_number) override;
        std::vector<Order> findAll() override;

    private:
        std::string generateOrderNumber();

        std::string file_path;
        IClock& clock;
};
