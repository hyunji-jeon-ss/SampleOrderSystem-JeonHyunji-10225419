#include "repository/JsonOrderRepository.h"

#include "clock/TimeFormat.h"
#include "model/OrderStatusText.h"
#include "repository/JsonFileStore.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

using nlohmann::json;

namespace
{
    Order fromJson(const json& element)
    {
        return Order{
            element.at("order_number").get<std::string>(),
            element.at("sample_id").get<std::string>(),
            element.at("customer_name").get<std::string>(),
            element.at("quantity").get<int>(),
            orderStatusFromString(element.at("status").get<std::string>()),
            element.at("shortage_quantity").get<int>(),
            element.at("enqueued_at_millis").get<long long>(),
            element.at("real_production_quantity").get<int>(),
            element.at("production_start_millis").get<long long>(),
            element.at("production_end_millis").get<long long>()
        };
    }

    json toJson(const Order& order)
    {
        return json{
            {"order_number", order.order_number},
            {"sample_id", order.sample_id},
            {"customer_name", order.customer_name},
            {"quantity", order.quantity},
            {"status", orderStatusToString(order.status)},
            {"shortage_quantity", order.shortage_quantity},
            {"enqueued_at_millis", order.enqueued_at_millis},
            {"real_production_quantity", order.real_production_quantity},
            {"production_start_millis", order.production_start_millis},
            {"production_end_millis", order.production_end_millis}
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

    writeJsonArray(file_path, root);

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
    const json root = readJsonArray(file_path);

    std::vector<Order> orders;
    orders.reserve(root.size());
    for (const json& element : root)
    {
        orders.push_back(fromJson(element));
    }
    return orders;
}
