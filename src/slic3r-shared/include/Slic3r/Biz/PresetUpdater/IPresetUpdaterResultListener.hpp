#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDescriptor.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterWarning.hpp"
#include <vector>
namespace Slic3r::Biz::PresetUpdater {

class IPresetUpdaterResultListener
{
public:
    virtual ~IPresetUpdaterResultListener() = default;

    virtual void on_preset_updater_error(
        const std::string& body
    ) = 0;
    virtual void on_preset_updater_forced_reconfigurations_list(
        const PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdaterWarning>& warnings
    ) = 0;
    virtual void on_preset_updater_reconfigurations_list(
        const PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdaterWarning>& warnings,
        VerboseStyle verbose
    ) = 0;
    virtual void on_preset_updater_reconfigurations_performed(
        const std::vector<PresetUpdaterWarning>& warnings
    ) {};
    virtual void on_preset_updater_status(
        const std::string& target, int attempt, unsigned delay, VerboseStyle verbose
    ) = 0;
    virtual void on_preset_updater_repository_info_vector(
        const SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdaterWarning>& warnings
    ) {};

    virtual void on_preset_updater_repository_selection_performed(
        const SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdaterWarning>& warnings
    ) {};
};
} // namespace Slic3r::Biz::PresetUpdater
