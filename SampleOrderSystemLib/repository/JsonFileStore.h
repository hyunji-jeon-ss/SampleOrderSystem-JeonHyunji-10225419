#pragma once

#include <nlohmann/json.hpp>

#include <string>

nlohmann::json readJsonArray(const std::string& file_path);
void writeJsonArray(const std::string& file_path, const nlohmann::json& array);
