#pragma once

#include "model/OrderStatus.h"

#include <string>

std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& text);
