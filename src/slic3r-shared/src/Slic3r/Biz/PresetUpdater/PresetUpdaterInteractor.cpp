#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "PresetUpdaterUtils.hpp"
#include "PresetUpdaterProcessStatus.hpp"
#include "PresetUpdaterRepositoryDatabase.hpp"
#include "PresetUpdaterRepositorySync.hpp"

#include "Slic3r/Log.hpp"

namespace Slic3r::Biz::PresetUpdater {

#if 0 // debug logging
namespace {
void log_reconfigurations(const PresetUpdaterReconfigurationList& reconfigurations)
{
    SPDLOG_INFO("Reconfigurations: updates: {} forced updates: {} downgrades: {}", std::to_string(reconfigurations.regular_updates().size()), std::to_string(reconfigurations.forced_updates().size()), std::to_string(reconfigurations.forced_downgrades().size()));
    for (const auto& reconf :reconfigurations.regular_updates()) {
        SPDLOG_INFO("update: {}", reconf.vendor_id);
    }
    for (const auto& reconf :reconfigurations.forced_updates()) {
        SPDLOG_INFO("forced update: {}", reconf.vendor_id);
    }
    for (const auto& reconf :reconfigurations.forced_downgrades()) {
        SPDLOG_INFO("forced downgrade: {}", reconf.vendor_id);
    }
}
} // namespace
#endif

PresetUpdaterInteractor::PresetUpdaterInteractor(Platform::IMainThreadDispatcher& dispatcher) :
    m_dispatcher(dispatcher)
{}

PresetUpdaterInteractor::~PresetUpdaterInteractor()
{
    ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
}

JThread::JThread PresetUpdaterInteractor::spawn_worker(std::function<void(JThread::StopToken)> body)
{
    return JThread::JThread([this, body = std::move(body)](JThread::StopToken stop_token) {
        try {
            body(stop_token);
        } catch (const std::exception& e) {
            SPDLOG_ERROR("PresetUpdater worker threw an unhandled exception: {}", e.what());
            dispatch_error(std::string("PresetUpdater operation failed with an unexpected exception: ") + e.what());
        } catch (...) {
            SPDLOG_ERROR("PresetUpdater worker threw an unhandled unknown exception.");
            dispatch_error("PresetUpdater operation failed with an unknown exception.");
        }
    });
}

void PresetUpdaterInteractor::check_forced_reconfigurations()
{
    //Does not care about App config value - allways runs
    // Forced reconfigurations must be checked on startup

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::NoProgress);
    };
    auto dispatch_suc = [this](const PresetUpdaterReconfigurationList& reconfigurations, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_forced_reconfigurations_list(reconfigurations, warnings);
    };

    m_thread = spawn_worker(
        [dispatch_err, dispatch_sta, dispatch_suc](JThread::StopToken stop_token) {
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
            PresetUpdaterRepositoryDatabase repo_database(&process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            const SharedRepositoryVector& repos = repo_database.get_selected_repositories();
            PresetUpdaterReconfigurationList reconfigurations = PresetUpdater::check_forced_reconfigurations(repos, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            dispatch_suc(reconfigurations, process_status.warnings());
        }
    );
}

void PresetUpdaterInteractor::build_update_sync_and_reconfiguration_check(bool app_config_preset_updater_allowed, VerboseStyle verbose, bool ignore_hash /*= false*/)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this, verbose](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, verbose);
    };
    auto dispatch_suc = [this, verbose](const PresetUpdaterReconfigurationList& reconfigurations, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_reconfigurations_list(reconfigurations, warnings, verbose);
    };

    m_thread = spawn_worker(
        [dispatch_err, dispatch_sta, dispatch_suc, ignore_hash](JThread::StopToken stop_token) {
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES, ignore_hash);
            PresetUpdaterRepositoryDatabase repo_database(&process_status);
            PresetUpdaterRepositorySync archive_sync;

            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }

            repo_database.sync(&process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            const SharedRepositoryVector& repos = repo_database.get_selected_repositories();
            archive_sync.sync(repos, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            PresetUpdaterReconfigurationList reconfigurations = PresetUpdater::check_reconfigurations(repos, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            dispatch_suc(reconfigurations, process_status.warnings());
        }
    );
}

void PresetUpdaterInteractor::perform_reconfigurations(
    const PresetUpdaterReconfigurationList& reconfigurations
)
{
    // Does not care about App config value - allways runs
    // Possible forced reconfigurations must be able to be performed

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::ProgressNotification);
    };
    auto dispatch_suc = [this](const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_reconfigurations_performed(warnings);
    };

    m_thread = spawn_worker(
        [reconfigurations, dispatch_err, dispatch_sta, dispatch_suc](JThread::StopToken stop_token) {
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);

            PresetUpdater::perform_reconfigurations(reconfigurations, ReconfigurationType::All, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            dispatch_suc(process_status.warnings());
        }
    );
}

void PresetUpdaterInteractor::update_repositories(bool app_config_preset_updater_allowed, const SharedPresetUpdaterRepositoryInfoVector& repos)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::ProgressNotification);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_repository_selection_performed(repos, warnings);
    };

    m_thread = spawn_worker([dispatch_err, dispatch_sta, dispatch_suc, repos](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
        PresetUpdaterRepositoryDatabase repo_database(&process_status);

        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        // If repos came in empty, it is a new query - we do sync with servers
        // Otherwise it is a selection from user - we just update selection
        if (repos.empty()) {
            repo_database.sync(&process_status);
        } else {
            repo_database.apply_selection(repos, &process_status);
        }
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        if (stop_token.stop_requested()) {
            return;
        }

        const SharedPresetUpdaterRepositoryInfoVector& repos = repo_database.get_all_repositories();

        dispatch_suc(repos, process_status.warnings());
    });
}

void PresetUpdaterInteractor::add_local_repository(bool app_config_preset_updater_allowed, const boost::filesystem::path& zip_path, bool unselect_others)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::ProgressNotification);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_repository_info_vector(repos, warnings);
    };

    m_thread = spawn_worker([dispatch_err, dispatch_sta, dispatch_suc, zip_path, unselect_others](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
        PresetUpdaterRepositoryDatabase repo_database(&process_status);

        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        repo_database.add_local_repository(zip_path, unselect_others, &process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        if (stop_token.stop_requested()) {
            return;
        }

        const SharedPresetUpdaterRepositoryInfoVector& repos = repo_database.get_all_repositories();

        dispatch_suc(repos, process_status.warnings());
    });
}

void PresetUpdaterInteractor::remove_local_repository(bool app_config_preset_updater_allowed, const std::string& uuid)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::ProgressNotification);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_repository_info_vector(repos, warnings);
    };

    m_thread = spawn_worker([dispatch_err, dispatch_sta, dispatch_suc, uuid](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
        PresetUpdaterRepositoryDatabase repo_database(&process_status);

        repo_database.remove_local_repository(uuid, &process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        if (stop_token.stop_requested()) {
            return;
        }

        const SharedPresetUpdaterRepositoryInfoVector& repos = repo_database.get_all_repositories();

        dispatch_suc(repos, process_status.warnings());
    });
}

void PresetUpdaterInteractor::list_repositories(bool app_config_preset_updater_allowed)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::ProgressNotification);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_repository_info_vector(repos, warnings);
    };

    m_thread = spawn_worker([dispatch_err, dispatch_sta, dispatch_suc](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
        PresetUpdaterRepositoryDatabase repo_database(&process_status);

        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        repo_database.sync(&process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        const SharedPresetUpdaterRepositoryInfoVector& repos = repo_database.get_all_repositories();

        dispatch_suc(repos, process_status.warnings());
    });
}

void PresetUpdaterInteractor::cleanup_update_sync(bool app_config_preset_updater_allowed)
{
    if (!app_config_preset_updater_allowed) {
        return;
    }

    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay, VerboseStyle::NoProgress);
    };
    auto dispatch_suc = [this](const PresetUpdaterReconfigurationList& reconfigurations, const std::vector<PresetUpdaterWarning>& warnings) {
        dispatch_reconfigurations_list(reconfigurations, warnings, VerboseStyle::NoProgress);
    };

    m_thread = spawn_worker([dispatch_err, dispatch_sta, dispatch_suc](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta, PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES);
        PresetUpdater::cleanup_update_sync(&process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        PresetUpdaterRepositoryDatabase repo_database(&process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        const SharedRepositoryVector& repos = repo_database.get_selected_repositories();
        PresetUpdaterReconfigurationList reconfigurations = PresetUpdater::check_reconfigurations(repos, &process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        dispatch_suc(reconfigurations, process_status.warnings());
    });
}

void PresetUpdaterInteractor::on_notification_cancel()
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }
}

void PresetUpdaterInteractor::dispatch_error(const std::string& body)
{
    m_dispatcher.dispatch_on_main_thread([this, body]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([body](auto* listener) {
            listener->on_preset_updater_error(body);
        });
    });
}

void PresetUpdaterInteractor::dispatch_status(const std::string& target, int attempt, unsigned delay, VerboseStyle verbose)
{
    m_dispatcher.dispatch_on_main_thread([this, target, attempt, delay, verbose]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([target, attempt, delay, verbose](auto* listener) {
            listener->on_preset_updater_status(target, attempt, delay, verbose);
        });
    });
}

void PresetUpdaterInteractor::dispatch_forced_reconfigurations_list(
    const PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<PresetUpdaterWarning>& warnings
)
{
    m_dispatcher.dispatch_on_main_thread([this, reconfigurations, warnings]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([reconfigurations, warnings](auto* listener) {
            listener->on_preset_updater_forced_reconfigurations_list(reconfigurations, warnings);
        });
    });
}

void PresetUpdaterInteractor::dispatch_reconfigurations_list(
    const PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<PresetUpdaterWarning>& warnings,
    VerboseStyle verbose
)
{
    m_dispatcher.dispatch_on_main_thread([this, reconfigurations, warnings, verbose]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([reconfigurations, warnings, verbose](auto* listener) {
            listener->on_preset_updater_reconfigurations_list(reconfigurations, warnings, verbose);
        });
    });
}

void PresetUpdaterInteractor::dispatch_reconfigurations_performed(const std::vector<PresetUpdaterWarning>& warnings)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, warnings]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([warnings](auto* listener) {
            listener->on_preset_updater_reconfigurations_performed(warnings);
        });
    });
}

void PresetUpdaterInteractor::dispatch_repository_info_vector(
    const SharedPresetUpdaterRepositoryInfoVector& repos,
    const std::vector<PresetUpdaterWarning>& warnings
)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, repos, warnings]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([repos, warnings](auto* listener) {
            listener->on_preset_updater_repository_info_vector(repos, warnings);
        });
    });
}

void PresetUpdaterInteractor::dispatch_repository_selection_performed(
    const SharedPresetUpdaterRepositoryInfoVector& repos,
    const std::vector<PresetUpdaterWarning>& warnings
)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, repos, warnings]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([repos, warnings](auto* listener) {
            listener->on_preset_updater_repository_selection_performed(repos, warnings);
        });
    });
}

} // namespace Slic3r::Biz::PresetUpdater
