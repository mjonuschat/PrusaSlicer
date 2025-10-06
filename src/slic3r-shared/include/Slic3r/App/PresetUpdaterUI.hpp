#pragma once

#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"

namespace Slic3r::App {

class PresetUpdaterUI : public Biz::PresetUpdater::IPresetUpdaterResultListener
{
public:
    PresetUpdaterUI(Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor);
    ~PresetUpdaterUI() = default;

    void on_preset_updater_error(const std::string& body) override;
    void on_preset_updater_reconfigurations_list(
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_reconfigurations_perfomed(const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings) override;
    void on_preset_updater_status(const std::string& target, int attempt, unsigned delay) override;
    void on_preset_updater_repository_info_vector(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;

private:
    Biz::PresetUpdater::PresetUpdaterInteractor& m_preset_updater_interactor;
};

} // namespace Slic3r::App