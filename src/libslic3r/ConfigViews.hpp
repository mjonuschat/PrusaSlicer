#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/FullConfigSLA.hpp"

namespace Slic3r {

inline std::vector<Domain::PartialConfigPtr> join(
    const Domain::PartialObjectConfigFDMPtr& object_settings,
    const std::vector<Domain::PartialVolumeConfigFDMPtr>& volume_settings
) {
    std::vector<Domain::PartialConfigPtr> result;
    result.push_back(object_settings);
    result.insert(result.end(), volume_settings.begin(), volume_settings.end());
    return result;
}

class PrintRegionConfigView : public Domain::ConfigView
{
public:
    PrintRegionConfigView(
        const Domain::FullConfigFDMPtr& full_config,
        const Domain::PartialObjectConfigFDMPtr& object_settings,
        const std::vector<Domain::PartialVolumeConfigFDMPtr>& volume_settings
    ):
        ConfigView{full_config, join(object_settings, volume_settings)}
    {}

    PrintRegionConfigView():
        ConfigView{std::make_shared<const Domain::FullConfigFDM>(Domain::FullConfigFDM::defaults()), {}}
    {}

    void add_override(const Domain::PartialVolumeConfigFDMPtr& override) {
        m_partial_configs.push_back(override);
    }

    std::size_t tools_count() const {
        return std::dynamic_pointer_cast<const Domain::FullConfigFDM>(m_full_config)->tools_count();
    }

    std::size_t filaments_count() const {
        return std::dynamic_pointer_cast<const Domain::FullConfigFDM>(m_full_config)->filaments_count();
    }

    const Domain::PartialVolumeConfigFDM volume_settings() const {
        return *std::dynamic_pointer_cast<const Domain::PartialVolumeConfigFDM>(m_partial_configs.back());
    }
};

class PrintObjectConfigView : public Domain::ConfigView
{
public:
    PrintObjectConfigView(
        const Domain::FullConfigFDMPtr& full_config,
        const Domain::PartialObjectConfigFDMPtr& object_settings
    ):
        ConfigView{full_config, {object_settings}}
    {}

    const Domain::PartialObjectConfigFDM& object_settings() const {
        return *std::dynamic_pointer_cast<const Domain::PartialObjectConfigFDM>(m_partial_configs.front());
    }
};

class PrintConfigView : public Domain::ConfigView
{
public:
    PrintConfigView(
        const Domain::FullConfigFDMPtr& full_config
    ):
        ConfigView{full_config, {}}
    {}

    PrintConfigView():
        ConfigView{std::make_shared<const Domain::FullConfigFDM>(Domain::FullConfigFDM::defaults()), {}}
    {}

    const Domain::FullConfigFDM& full_config() const {
        return *std::dynamic_pointer_cast<const Domain::FullConfigFDM>(m_full_config);
    }
};

class SLAPrintConfigView : public Domain::ConfigView
{
public:
    SLAPrintConfigView(
        const Domain::FullConfigSLAPtr& full_config
    ):
        ConfigView{full_config, {}}
    {}

    SLAPrintConfigView():
        ConfigView{std::make_shared<const Domain::FullConfigSLA>(Domain::FullConfigSLA::defaults()), {}}
    {}

    const Domain::FullConfigSLA& full_config() const {
        return *std::dynamic_pointer_cast<const Domain::FullConfigSLA>(m_full_config);
    }
};

class SLAPrintObjectConfigView : public Domain::ConfigView
{
public:
    SLAPrintObjectConfigView(
        const Domain::FullConfigSLAPtr& full_config,
        const Domain::PartialObjectConfigSLAPtr& object_settings
    ):
        ConfigView{full_config, {object_settings}}
    {}

    const Domain::PartialObjectConfigSLA& object_settings() const {
        return *std::dynamic_pointer_cast<const Domain::PartialObjectConfigSLA>(m_partial_configs.front());
    }
};

} // namespace Slic3r
