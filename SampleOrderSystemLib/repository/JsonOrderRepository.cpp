#include "repository/JsonOrderRepository.h"

#include "clock/TimeFormat.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

using nlohmann::json;

namespace
{
    std::string orderStatusToString(OrderStatus status)
    {
        switch (status)
        {
            case OrderStatus::RESERVED: return "RESERVED";
            case OrderStatus::REJECTED: return "REJECTED";
            case OrderStatus::PRODUCING: return "PRODUCING";
            case OrderStatus::CONFIRMED: return "CONFIRMED";
            case OrderStatus::RELEASED: return "RELEASED";
        }
        return "RESERVED";
    }

    OrderStatus orderStatusFromString(const std::string& text)
    {
        if (text == "REJECTED") return OrderStatus::REJECTED;
        if (text == "PRODUCING") return OrderStatus::PRODUCING;
        if (text == "CONFIRMED") return OrderStatus::CONFIRMED;
        if (text == "RELEASED") return OrderStatus::RELEASED;
        return OrderStatus::RESERVED;
    }

    Order fromJson(const json& element)
    {
        return Order{
            element.at("order_number").get<std::string>(),
            element.at("sample_id").get<std::string>(),
            element.at("customer_name").get<std::string>(),
            element.at("quantity").get<int>(),
            orderStatusFromString(element.at("status").get<std::string>())
        };
    }

    json toJson(const Order& order)
    {
        return json{
            {"order_number", order.order_number},
            {"sample_id", order.sample_id},
            {"customer_name", order.customer_name},
            {"quantity", order.quantity},
            {"status", orderStatusToString(order.status)}
        };
    }
}

JsonOrderRepository::JsonOrderRepository(const std::string& file_path, IClock& clock)
    : file_path(file_path)
    , clock(clock)
{
}

std::string JsonOrderRepository::generateOrderNumber()
{
    const std::string today = formatDate(clock.nowMillis());
    const std::string prefix = "ORD-" + today + "-";

    int max_sequence = 0;
    for (const Order& order : findAll())
    {
        if (order.order_number.rfind(prefix, 0) != 0) continue;

        try
        {
            const int sequence = std::stoi(order.order_number.substr(prefix.size()));
            if (sequence > max_sequence) max_sequence = sequence;
        }
        catch (const std::exception&)
        {
        }
    }

    std::ostringstream oss;
    oss << prefix << std::setw(4) << std::setfill('0') << (max_sequence + 1);
    return oss.str();
}

Order JsonOrderRepository::save(const Order& order)
{
    std::vector<Order> orders = findAll();

    Order saved_order = order;
    if (saved_order.order_number.empty())
    {
        saved_order.order_number = generateOrderNumber();
        orders.push_back(saved_order);
    }
    else
    {
        auto it = std::find_if(orders.begin(), orders.end(),
            [&saved_order](const Order& existing) { return existing.order_number == saved_order.order_number; });

        if (it == orders.end())
        {
            orders.push_back(saved_order);
        }
        else
        {
            *it = saved_order;
        }
    }

    json root = json::array();
    for (const Order& stored_order : orders)
    {
        root.push_back(toJson(stored_order));
    }

    std::ofstream output(file_path);
    output << root.dump(4);

    return saved_order;
}

std::optional<Order> JsonOrderRepository::findByOrderNumber(const std::string& order_number)
{
    for (const Order& order : findAll())
    {
        if (order.order_number == order_number) return order;
    }
    return std::nullopt;
}

std::vector<Order> JsonOrderRepository::findAll()
{
    std::ifstream input(file_path);
    if (!input.is_open()) return {};

    input.seekg(0, std::ios::end);
    if (input.tellg() == 0) return {};
    input.seekg(0, std::ios::beg);

    json root;
    input >> root;

    std::vector<Order> orders;
    orders.reserve(root.size());
    for (const json& element : root)
    {
        orders.push_back(fromJson(element));
    }
    return orders;
}
