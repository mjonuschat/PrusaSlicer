#include "Slic3r/App/PresetUpdaterUI.hpp"

#include "Slic3r/Log.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfig.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <fmt/format.h>
#include <string_view>

namespace Slic3r::App {

PresetUpdaterUI::PresetUpdaterUI(
    Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor,
    Biz::Preset::PresetInteractor& preset_interactor,
    Navigator& navigator,
    const Biz::Preset::IO::BundlePaths& bundle_paths
) :
    m_preset_updater_interactor(preset_updater_interactor),
    m_preset_interactor(preset_interactor),
    m_navigator(navigator),
    m_bundle_paths(bundle_paths)
{
    m_preset_updater_interactor.add_listener<Biz::PresetUpdater::IPresetUpdaterResultListener>(
        this
    );
    m_preset_updater_interactor.check_forced_reconfigurations();
}

void PresetUpdaterUI::on_preset_updater_error(Biz::PresetUpdater::JobId job_id, const std::string& body)
{
    SPDLOG_INFO("{}: {}", std::string(__FUNCTION__), body);
    DEBUG_ASSERT(false);
}

void PresetUpdaterUI::on_preset_updater_forced_reconfigurations_list(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (!warnings.empty()) {
        constexpr std::string_view prefix = "Preset updater warnings:\n";
        size_t total_len                  = prefix.length();
        for (const auto& warning : warnings) {
            total_len += warning.string().length() + 1;
        }

        std::string warn_text;
        warn_text.reserve(total_len);
        warn_text.assign(prefix);
        for (const auto& warning : warnings) {
            warn_text.append(warning.string());
            warn_text += '\n';
        }
        SPDLOG_WARN(warn_text);

        AppServices::instance().pop_notification_center().show_preset_updater_warnings(warnings);
    }
    SPDLOG_INFO(__FUNCTION__);
    SPDLOG_INFO(
        "Forced Reconfigurations check: forced updates: {} forced downgrades: {}",
        std::to_string(reconfigurations.forced_updates().size()),
        std::to_string(reconfigurations.forced_downgrades().size())
    );
    for (const auto& reconf : reconfigurations.forced_updates()) {
        SPDLOG_INFO("forced update: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.forced_downgrades()) {
        SPDLOG_INFO("forced downgrade: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }

    if (!reconfigurations.forced_updates().empty() || !reconfigurations.forced_downgrades().empty())
    {
        Biz::PresetUpdater::PresetUpdaterReconfigurationList reconfigurations_to_perform{
            {},
            reconfigurations.forced_updates(),
            reconfigurations.forced_downgrades(),
            {},
            {}
        };
        auto callback = [this, reconf = std::move(reconfigurations_to_perform)](bool answer)
        {
            if (answer) {
                m_preset_updater_interactor.perform_reconfigurations(reconf);
            } else {
                m_navigator.close_application();
            }
        };
        AppServices::instance().dialog_manager().show_forced_reconfigurations_dialog(
            reconfigurations,
            callback
        );
        return;
    }

    // Now, installed vendors were checked for forced reconfigurations.
    // It is time to do online background update check
    m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(
        AppServices::instance().app_config().get<bool>("enable_preset_update"),
        Biz::PresetUpdater::VerboseStyle::NoProgress
    );
    return;
}

void PresetUpdaterUI::on_preset_updater_reconfigurations_list(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings,
    Biz::PresetUpdater::VerboseStyle verbose
)
{
    if (!warnings.empty()) {
        constexpr std::string_view prefix = "Preset updater warnings:\n";
        size_t total_len                  = prefix.length();
        for (const auto& warning : warnings) {
            total_len += warning.string().length() + 1;
        }

        std::string warn_text;
        warn_text.reserve(total_len);
        warn_text.assign(prefix);
        for (const auto& warning : warnings) {
            warn_text.append(warning.string());
            warn_text += '\n';
        }
        SPDLOG_WARN(warn_text);
        if (verbose == Biz::PresetUpdater::VerboseStyle::ProgressNotification) {
            AppServices::instance().pop_notification_center().show_preset_updater_warnings(
                warnings
            );
        }
    }

    SPDLOG_INFO(__FUNCTION__);
    SPDLOG_INFO(
        "Reconfigurations: updates: {} forced updates: {} downgrades: {}",
        std::to_string(reconfigurations.regular_updates().size()),
        std::to_string(reconfigurations.forced_updates().size()),
        std::to_string(reconfigurations.forced_downgrades().size())
    );
    for (const auto& reconf : reconfigurations.regular_updates()) {
        SPDLOG_INFO("update: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.new_vendors()) {
        SPDLOG_INFO("new vendor: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.not_in_index()) {
        SPDLOG_INFO("not in index: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.forced_updates()) {
        SPDLOG_INFO("forced update: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }
    for (const auto& reconf : reconfigurations.forced_downgrades()) {
        SPDLOG_INFO("forced downgrade: {}/{}", reconf.vendor_repo_id, reconf.vendor_id);
    }

    // Result is empty - show info if verbose mode
    if (reconfigurations.empty()) {
        if (verbose == Biz::PresetUpdater::VerboseStyle::ProgressNotification) {
            AppServices::instance()
                .pop_notification_center()
                .show_preset_updater_no_reconfigurations();
        }
        return;
    }

    // Result has forced reconfigurations - promts user in modal dialog.
    if (!reconfigurations.forced_updates().empty() || !reconfigurations.forced_downgrades().empty())
    {
        Biz::PresetUpdater::PresetUpdaterReconfigurationList reconfigurations_to_perform{
            {},
            reconfigurations.forced_updates(),
            reconfigurations.forced_downgrades(),
            {},
            {}
        };
        auto callback = [this, reconf = std::move(reconfigurations_to_perform)](bool answer)
        {
            if (answer) {
                m_preset_updater_interactor.perform_reconfigurations(reconf);
            } else {
                m_navigator.close_application();
            }
        };

        AppServices::instance().dialog_manager().show_forced_reconfigurations_dialog(
            reconfigurations,
            callback
        );
        return;
    }

    // Result has only Not in Index reconfigurations (which is currently not concidered as empty) - show info if verbose mode
    if (reconfigurations.new_vendors().empty() && reconfigurations.regular_updates().empty()) {
        if (verbose == Biz::PresetUpdater::VerboseStyle::ProgressNotification) {
            AppServices::instance()
                .pop_notification_center()
                .show_preset_updater_no_reconfigurations();
        }
        return;
    }

    // Result has regular reconfigurations - show notification
    auto callback = [this, reconfigurations]()
    {
        m_preset_updater_interactor.perform_reconfigurations(
            {reconfigurations.regular_updates(), {}, {}, reconfigurations.new_vendors(), {}}
        );
    };
    AppServices::instance().pop_notification_center().show_preset_updater_reconfigurations_list(
        reconfigurations,
        callback
    );
}

void PresetUpdaterUI::on_preset_updater_reconfigurations_performed(
    Biz::PresetUpdater::JobId job_id,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (!warnings.empty()) {
        AppServices::instance().pop_notification_center().show_preset_updater_warnings(warnings);
    }
    SPDLOG_INFO(__FUNCTION__);
    // TODO: detect if reload is needed
    SPDLOG_INFO("Update finished, Reloading presets");
    m_preset_interactor.load_preset_bundle(m_bundle_paths);
}

void PresetUpdaterUI::on_preset_updater_status(
    Biz::PresetUpdater::JobId job_id,
    const std::string& target,
    int attempt,
    unsigned delay,
    Biz::PresetUpdater::VerboseStyle verbose
)
{
    SPDLOG_INFO(
        "PRESET UPDATER STATUS: target:{} attempt:{} delay:{}",
        target,
        std::to_string(attempt),
        std::to_string(delay)
    );
}

void PresetUpdaterUI::on_preset_updater_repository_info_vector(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (!warnings.empty()) {
        AppServices::instance().pop_notification_center().show_preset_updater_warnings(warnings);
    }
    AppServices::instance().pop_notification_center().observable_list().close_notifications_of_type(
        PopNotification::PopNotificationType::PresetUpdaterStatus
    );

    Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector result =
        AppServices::instance().dialog_manager().show_preset_sources_dialog(descriptor);
    if (!result.empty()) {
        m_preset_updater_interactor.apply_repository_selection(
            AppServices::instance().app_config().get<bool>("enable_preset_update"),
            result
        );
    }
}

void PresetUpdaterUI::on_preset_updater_repository_selection_performed(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (!warnings.empty()) {
        AppServices::instance().pop_notification_center().show_preset_updater_warnings(warnings);
    }
    // TODO: show notification with warnings
    m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(
        AppServices::instance().app_config().get<bool>("enable_preset_update"),
        Biz::PresetUpdater::VerboseStyle::ProgressNotification
    );
}

} // namespace Slic3r::App
