#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigFDM.hpp"

namespace Slic3r {

inline std::vector<Domain::ConfigBoxPtr> join(
    const Domain::ObjectSettingsPtr& object_settings,
    const std::vector<Domain::VolumeSettingsPtr>& volume_settings
) {
    std::vector<Domain::ConfigBoxPtr> result;
    result.push_back(object_settings);
    result.insert(result.end(), volume_settings.begin(), volume_settings.end());
    return result;
}

class PrintRegionConfigView : public Domain::ConfigView
{
public:
    PrintRegionConfigView(
        const Domain::FullConfigPtr& full_config,
        const Domain::ObjectSettingsPtr& object_settings,
        const std::vector<Domain::VolumeSettingsPtr>& volume_settings
    ):
        ConfigView{full_config, join(object_settings, volume_settings)}
    {}

    PrintRegionConfigView():
        ConfigView{std::make_shared<const Domain::FullConfigFDM>(Domain::FullConfigFDM::defaults()), {}}
    {}

    void add_override(const Domain::VolumeSettingsPtr& override) {
        m_config_boxes.push_back(override);
    }
};

class PrintObjectConfigView : public Domain::ConfigView
{
public:
    PrintObjectConfigView(
        const Domain::FullConfigPtr& full_config,
        const Domain::ObjectSettingsPtr& object_settings
    ):
        ConfigView{full_config, {object_settings}}
    {}

    Domain::ObjectSettingsPtr object_settings() const {
        return std::dynamic_pointer_cast<const Domain::ObjectSettings>(m_config_boxes.front());
    }
};

class PrintConfigView : public Domain::ConfigView
{
public:
    PrintConfigView(
        const Domain::FullConfigPtr& full_config
    ):
        ConfigView{full_config, {}}
    {}

    PrintConfigView():
        ConfigView{std::make_shared<const Domain::FullConfigFDM>(Domain::FullConfigFDM::defaults()), {}}
    {}
};

} // namespace Slic3r
