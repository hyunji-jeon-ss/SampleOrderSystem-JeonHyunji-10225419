#pragma once

#include "repository/ISampleRepository.h"

#include <string>

class JsonSampleRepository : public ISampleRepository
{
    public:
        explicit JsonSampleRepository(const std::string& file_path);

        Sample save(const Sample& sample) override;
        std::optional<Sample> findById(const std::string& id) override;
        std::vector<Sample> findAll() override;

    private:
        std::string file_path;
        int next_sequence = 1;
};
