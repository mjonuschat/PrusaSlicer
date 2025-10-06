#pragma once

#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/App/Init.hpp"

namespace Slic3r::App {

class PresetUpdaterCLI : public Biz::PresetUpdater::IPresetUpdaterResultListener
{
public:
    PresetUpdaterCLI(Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor);

    void start(const ActionParams& action, const std::string data);

    void on_preset_updater_error(const std::string& body) override;

    void on_preset_updater_reconfigurations_list(
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;

    void on_preset_updater_reconfigurations_perfomed(
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;

    void on_preset_updater_status(const std::string& target, int attempt, unsigned delay) override;

    void on_preset_updater_repository_info_vector(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;

    bool has_result()
    {
        return m_has_result;
    }

private:
    Biz::PresetUpdater::PresetUpdaterInteractor& m_preset_updater_interactor;
    bool m_has_result{false};
    bool m_perform_after_sync{false};
    std::string m_repo_to_switch;
};

} // namespace Slic3r::App
