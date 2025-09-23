#include "Slic3r/Biz/Config/FeatureStructurizer.hpp"

#include <string_view>
#include <algorithm>
#include <fmt/format.h>

namespace Slic3r::Biz::Config {

namespace {
void push_key_value(Domain::JsonObject& dest, std::string_view key, const Domain::JsonValue& value)
{
    auto idx = key.find('.');
    if (idx != std::string_view::npos) {
        std::string current_key{key.substr(0, idx)};
        if (!dest.contains(current_key))
            dest[current_key] = Domain::JsonObject{};
        push_key_value(std::get<Domain::JsonObject>(dest[current_key]), key.substr(current_key.length() + 1), value);
    } else {
        dest[std::string{key}] = value;
    }
}

void pull_key_values(
    Domain::Preset::FeatureValueMap& dest,
    const std::string& key_prefix,
    const Domain::JsonObject& values
)
{
    for (const auto& [k, v] : values) {
        std::string prefixed_key = key_prefix + k;
        if (std::holds_alternative<Domain::JsonObject>(v))
            pull_key_values(dest, prefixed_key + ".", std::get<Domain::JsonObject>(v));
        else
            dest[prefixed_key] = v;
    }
}

const std::string STRUCTURIZE_PREFIX = "$.";

}

Domain::JsonValue features_to_structure(const Domain::Preset::FeatureValueMap& features)
{
    Domain::JsonObject ret;
    for (const auto& [k, v] : features) {
        if (!k.starts_with(STRUCTURIZE_PREFIX))
            continue;
        push_key_value(ret, k.substr(STRUCTURIZE_PREFIX.length()), v);
    }
    return ret;
}

Domain::Preset::FeatureValueMap structure_to_features(const Domain::JsonValue& features)
{
    Domain::Preset::FeatureValueMap ret;
    pull_key_values(ret, STRUCTURIZE_PREFIX, std::get<Domain::JsonObject>(features));

    return ret;
}

void remove_structurize_features(Domain::Preset::FeatureValueMap& features)
{
    std::erase_if(
        features,
        [](const auto& kv) { return kv.first.starts_with(STRUCTURIZE_PREFIX); }
    );
}


}
