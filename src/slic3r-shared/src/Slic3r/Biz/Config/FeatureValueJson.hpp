#pragma once

#include <nlohmann/json.hpp>
#include "Slic3r/Domain/Preset/Types.hpp"

NLOHMANN_JSON_NAMESPACE_BEGIN


template<>
struct adl_serializer<Slic3r::Domain::Preset::FeatureValue>
{
    using FeatureValue = Slic3r::Domain::Preset::FeatureValue;

    static void to_json(ordered_json& j, const FeatureValue& v)
    {
        std::visit([&j](auto&& arg) { j = arg; }, v);
    }

    static void from_json(const ordered_json& j, FeatureValue& v)
    {
        if (j.is_boolean())
            v = j.get<bool>();
        else if (j.is_number())
            v = j.get<double>();
        else if (j.is_string())
            v = j.get<std::string>();
    }
};
NLOHMANN_JSON_NAMESPACE_END
