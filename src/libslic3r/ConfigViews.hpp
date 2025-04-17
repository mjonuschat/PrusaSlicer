#pragma once

#include <ranges>
#include <string_view>
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigFDM.hpp"

namespace Slic3r {

namespace {
    template <typename T, typename V>
    std::vector<Domain::ConfigBoxPtr> append_and_copy(const std::vector<T>& vector, const V& value) {
        std::vector<Domain::ConfigBoxPtr> result;
        result.reserve(vector.size() + 1);
        for (const auto& value : vector) {
            result.push_back(value);
        }
        result.push_back(value);
        return result;
    }
}

class PrintRegionConfig : public Domain::ConfigView
{
public:
    PrintRegionConfig(
        const Domain::FullConfigPtr& full_config,
        const std::shared_ptr<Domain::ObjectSettings>& object_settings,
        const std::vector<std::shared_ptr<Domain::VolumeSettings>>& volume_settings
    ):
        ConfigView{full_config, append_and_copy(volume_settings, object_settings)}
    {}
};

class PrintObjectConfig : public Domain::ConfigView
{
public:
    PrintObjectConfig(
        const Domain::FullConfigPtr& full_config,
        const std::shared_ptr<Domain::ObjectSettings>& object_settings
    ):
        ConfigView{full_config, {object_settings}}
    {}
};

// TEMPORARY class to translate configs
class PrintConfig : public Domain::ConfigView
{
public:
    PrintConfig(
        const Domain::FullConfigPtr& full_config
    ):
        ConfigView{full_config, {}}
    {}
};

} // namespace Slic3r
