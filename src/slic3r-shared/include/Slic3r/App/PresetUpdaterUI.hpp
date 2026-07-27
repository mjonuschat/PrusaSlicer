#pragma once

#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::App {
class Navigator;

class PresetUpdaterUI : public Biz::PresetUpdater::IPresetUpdaterResultListener
{
public:
    PresetUpdaterUI(
        Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor,
        Biz::Preset::PresetInteractor& preset_interactor,
        Navigator& navigator,
        const Biz::Preset::IO::BundlePaths& bundle_paths
    );
    ~PresetUpdaterUI() = default;

    void on_preset_updater_error(const std::string& body) override;
    void on_preset_updater_forced_reconfigurations_list(
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_reconfigurations_list(
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings,
        Biz::PresetUpdater::VerboseStyle verbose
    ) override;
    void on_preset_updater_reconfigurations_performed(const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings) override;
    void on_preset_updater_status(const std::string& target, int attempt, unsigned delay, Biz::PresetUpdater::VerboseStyle verbose) override;
    void on_preset_updater_repository_info_vector(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_repository_selection_performed(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector< Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    
private:
    Biz::PresetUpdater::PresetUpdaterInteractor& m_preset_updater_interactor;
    Biz::Preset::PresetInteractor& m_preset_interactor;
    Navigator& m_navigator;
    Biz::Preset::IO::BundlePaths m_bundle_paths;
};

} // namespace Slic3r::App