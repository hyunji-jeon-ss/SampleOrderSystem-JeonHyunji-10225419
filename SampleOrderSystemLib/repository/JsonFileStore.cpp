#include "repository/JsonFileStore.h"

#include <fstream>

using nlohmann::json;

json readJsonArray(const std::string& file_path)
{
    std::ifstream input(file_path);
    if (!input.is_open()) return json::array();

    input.seekg(0, std::ios::end);
    if (input.tellg() == 0) return json::array();
    input.seekg(0, std::ios::beg);

    json root;
    input >> root;
    return root;
}

void writeJsonArray(const std::string& file_path, const json& array)
{
    std::ofstream output(file_path);
    output << array.dump(4);
}
