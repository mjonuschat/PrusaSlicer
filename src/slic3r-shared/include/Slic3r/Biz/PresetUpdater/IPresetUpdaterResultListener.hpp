#pragma once

#include "Slic3r\Biz\PresetUpdater\PresetUpdaterReconfigurationList.hpp"
#include "Slic3r\Biz\PresetUpdater\PresetUpdaterRepositoryCredentials.hpp"

namespace Slic3r::Biz::PresetUpdater {

class IPresetUpdaterResultListener {
public:
    virtual ~IPresetUpdaterResultListener() = default;

    virtual void on_preset_updater_error(const std::string& body) = 0;
    virtual void on_preset_updater_reconfigurations_list(const PresetUpdaterReconfigurationList& reconfigurations) = 0;
    virtual void on_preset_updater_reconfigurations_perfomed() = 0;
    virtual void on_preset_updater_status(const std::string& target, int attempt, unsigned delay) = 0;
    virtual void on_preset_updater_repository_info_vector(const SharedPresetUpdaterRepositoryInfoVector& descriptor) = 0;

};
}