#include "repository/JsonSampleRepository.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

using nlohmann::json;

namespace
{
    Sample fromJson(const json& element)
    {
        return Sample{
            element.at("id").get<std::string>(),
            element.at("name").get<std::string>(),
            element.at("avg_production_time_min").get<double>(),
            element.at("yield").get<double>(),
            element.at("physical_stock").get<int>(),
            element.at("available_stock").get<int>()
        };
    }

    json toJson(const Sample& sample)
    {
        return json{
            {"id", sample.id},
            {"name", sample.name},
            {"avg_production_time_min", sample.avg_production_time_min},
            {"yield", sample.yield},
            {"physical_stock", sample.physical_stock},
            {"available_stock", sample.available_stock}
        };
    }

    std::string formatSampleId(int sequence)
    {
        std::ostringstream oss;
        oss << "S-" << std::setw(3) << std::setfill('0') << sequence;
        return oss.str();
    }

    int extractSequence(const std::string& id)
    {
        const std::string prefix = "S-";
        if (id.rfind(prefix, 0) != 0) return 0;

        try
        {
            return std::stoi(id.substr(prefix.size()));
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }
}

JsonSampleRepository::JsonSampleRepository(const std::string& file_path)
    : file_path(file_path)
{
    for (const Sample& sample : findAll())
    {
        const int sequence = extractSequence(sample.id);
        if (sequence >= next_sequence) next_sequence = sequence + 1;
    }
}

Sample JsonSampleRepository::save(const Sample& sample)
{
    std::vector<Sample> samples = findAll();

    Sample saved_sample = sample;
    if (saved_sample.id.empty())
    {
        saved_sample.id = formatSampleId(next_sequence);
        next_sequence++;
        samples.push_back(saved_sample);
    }
    else
    {
        auto it = std::find_if(samples.begin(), samples.end(),
            [&saved_sample](const Sample& existing) { return existing.id == saved_sample.id; });

        if (it == samples.end())
        {
            samples.push_back(saved_sample);
        }
        else
        {
            *it = saved_sample;
        }
    }

    json root = json::array();
    for (const Sample& stored_sample : samples)
    {
        root.push_back(toJson(stored_sample));
    }

    std::ofstream output(file_path);
    output << root.dump(4);

    return saved_sample;
}

std::optional<Sample> JsonSampleRepository::findById(const std::string& id)
{
    for (const Sample& sample : findAll())
    {
        if (sample.id == id) return sample;
    }
    return std::nullopt;
}

std::vector<Sample> JsonSampleRepository::findAll()
{
    std::ifstream input(file_path);
    if (!input.is_open()) return {};

    input.seekg(0, std::ios::end);
    if (input.tellg() == 0) return {};
    input.seekg(0, std::ios::beg);

    json root;
    input >> root;

    std::vector<Sample> samples;
    samples.reserve(root.size());
    for (const json& element : root)
    {
        samples.push_back(fromJson(element));
    }
    return samples;
}
