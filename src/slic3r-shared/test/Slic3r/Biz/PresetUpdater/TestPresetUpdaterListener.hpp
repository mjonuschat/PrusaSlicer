#pragma once

#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/App/Init.hpp"

namespace Slic3r::Biz::PresetUpdater {

enum class ReconfigurationResult
{
    Update, 
    ForcedUpdate,
    ForcedDowngrade,
    NotInIndex,
    NewVendor,
    None,
    Error,
};

struct ExpectedReconfiguration
{
    ReconfigurationResult state;
    Semver version;
    bool allow_warnings;
};

class TestPresetUpdaterListener : public IPresetUpdaterResultListener
{
public:
    TestPresetUpdaterListener(PresetUpdaterInteractor& preset_updater_interactor);

    void start(ExpectedReconfiguration expected);

    void on_preset_updater_error(const std::string& body) override;

    void on_preset_updater_forced_reconfigurations_list(
        const PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdaterWarning>& warnings
    ) override;

    void on_preset_updater_reconfigurations_list(
        const PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdaterWarning>& warnings,
        Biz::PresetUpdater::VerboseStyle verbose
    ) override;

    void on_preset_updater_reconfigurations_performed(
        const std::vector<PresetUpdaterWarning>& warnings
    ) override;

    void on_preset_updater_status(const std::string& target, int attempt, unsigned delay, Biz::PresetUpdater::VerboseStyle verbose) override;

    void on_preset_updater_repository_info_vector(
        const SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdaterWarning>& warnings
    ) override;

    bool has_result()
    {
        return m_has_result;
    }

private:
    PresetUpdaterInteractor& m_preset_updater_interactor;
    bool m_has_result{false};
    ExpectedReconfiguration m_expected;
};

} // namespace Slic3r::Biz::PresetUpdater