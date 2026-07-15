#pragma once

#include "model/Sample.h"

#include <optional>
#include <string>
#include <vector>

class ISampleRepository
{
    public:
        virtual ~ISampleRepository() = default;

        virtual Sample save(const Sample& sample) = 0;
        virtual std::optional<Sample> findById(const std::string& id) = 0;
        virtual std::vector<Sample> findAll() = 0;
};
