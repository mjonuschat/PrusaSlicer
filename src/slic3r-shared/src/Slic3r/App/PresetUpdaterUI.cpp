#include "Slic3r/App/PresetUpdaterUI.hpp"

#include "Slic3r/Log.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

#include <fmt/format.h>

namespace Slic3r::App {

PresetUpdaterUI::PresetUpdaterUI(
    Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor,
    Biz::Preset::PresetInteractor& preset_interactor,
    const Biz::Preset::IO::BundlePaths& bundle_paths
) :
    m_preset_updater_interactor(preset_updater_interactor),
    m_preset_interactor(preset_interactor),
    m_bundle_paths(bundle_paths)
{
    m_preset_updater_interactor.add_listener<Biz::PresetUpdater::IPresetUpdaterResultListener>(this);

    // TODO: uncomment this to enable remote downloading of presets
    // m_preset_updater_interactor.build_update_sync_and_reconfiguration_check();

    // Testing of functions
    // m_preset_updater_interactor.check_forced_reconfigurations();
    // Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector descriptor;
    // m_preset_updater_interactor.update_repositories(descriptor);
}

void PresetUpdaterUI::on_preset_updater_error(const std::string& body)
{
    SPDLOG_INFO("{}: {}", std::string(__FUNCTION__), body);
    DEBUG_ASSERT(false);
}

void PresetUpdaterUI::on_preset_updater_reconfigurations_list(
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    SPDLOG_INFO(__FUNCTION__);
    SPDLOG_INFO(
        "Reconfigurations: updates: {} forced updates: {} downgrades: {}",
        std::to_string(reconfigurations.regular_updates().size()),
        std::to_string(reconfigurations.forced_updates().size()),
        std::to_string(reconfigurations.forced_downgrades().size())
    );
    for (const auto& reconf : reconfigurations.regular_updates()) {
        SPDLOG_INFO("update: {}", reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.forced_updates()) {
        SPDLOG_INFO("forced update: {}", reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.forced_downgrades()) {
        SPDLOG_INFO("forced downgrade: {}", reconf.vendor_id);
    }

    std::string dialog_msg = fmt::format(
        "Preset Updater returned these reconfigurations:\n\nnew vendors: {}\nupdates: {}\nforced updates: {}\nforced downgrades: {}\n\n",
        std::to_string(reconfigurations.new_vendors().size()),
        std::to_string(reconfigurations.regular_updates().size()),
        std::to_string(reconfigurations.forced_updates().size()),
        std::to_string(reconfigurations.forced_downgrades().size())
    );

    for (const auto& reconf : reconfigurations.regular_updates()) {
        dialog_msg += fmt::format("update: {}\n", reconf.vendor_id);
    }
    dialog_msg += "\n";
    for (const auto& reconf : reconfigurations.forced_updates()) {
        dialog_msg += fmt::format("forced update: {}\n", reconf.vendor_id);
    }
    dialog_msg += "\n";
    for (const auto& reconf : reconfigurations.forced_downgrades()) {
        dialog_msg += fmt::format("forced downgrade: {}\n", reconf.vendor_id);
    }

    auto after_dialog = [this, reconfigurations](bool answer)
    {
        if (answer) {
            m_preset_updater_interactor.perform_reconfigurations(reconfigurations);
        }
    };
    AppServices::instance().dialog_manager().show_yesno_dialog("Preset Updater reconfigurations", dialog_msg, after_dialog);
}

void PresetUpdaterUI::on_preset_updater_reconfigurations_perfomed(
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    SPDLOG_INFO(__FUNCTION__);
    // TODO: detect if reload is needed
    SPDLOG_INFO("Update finished, Reloading presets");
    m_preset_interactor.load_preset_bundle(m_bundle_paths);
}

void PresetUpdaterUI::on_preset_updater_status(const std::string& target, int attempt, unsigned delay)
{
    SPDLOG_INFO("PRESET UPDATER STATUS: target:{} attempt:{} delay:{}", target, std::to_string(attempt), std::to_string(delay));
}

void PresetUpdaterUI::on_preset_updater_repository_info_vector(
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    std::string dialog_msg = "Preset Updater Sources\n\n";

    for (const auto& pair : descriptor) {
        dialog_msg += fmt::
            format("source: {}({}) {}\n", pair.descriptor.id, pair.descriptor.unzipped_data_path.empty() ? "online" : "local", pair.selected ? "selected" : "not selected");
    }

    AppServices::instance().dialog_manager().show_yesno_dialog("Preset Updater Sources", dialog_msg, nullptr);
}
} // namespace Slic3r::App
