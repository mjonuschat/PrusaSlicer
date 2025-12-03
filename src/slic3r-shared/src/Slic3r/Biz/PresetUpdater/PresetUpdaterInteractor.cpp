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

void PresetUpdaterInteractor::check_forced_reconfigurations()
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this](const PresetUpdaterReconfigurationList& reconfigurations) {
        dispatch_reconfigurations_list(reconfigurations);
    };

    m_thread = JThread::JThread(
        [dispatch_err, dispatch_sta, dispatch_suc](JThread::StopToken stop_token) {
            PresetUpdaterReconfigurationList reconfigurations;
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);
            PresetUpdater::check_forced_reconfigurations(reconfigurations, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            dispatch_suc(reconfigurations);
        }
    );
}

void PresetUpdaterInteractor::build_update_sync_and_reconfiguration_check()
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this](const PresetUpdaterReconfigurationList& reconfigurations) {
        dispatch_reconfigurations_list(reconfigurations);
    };

    m_thread = JThread::JThread(
        [dispatch_err, dispatch_sta, dispatch_suc](JThread::StopToken stop_token) {
            PresetUpdaterReconfigurationList reconfigurations;
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);
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

            PresetUpdater::check_reconfigurations(reconfigurations, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            if (stop_token.stop_requested()) {
                return;
            }

            dispatch_suc(reconfigurations);
        }
    );
}

void PresetUpdaterInteractor::perform_reconfigurations(
    const PresetUpdaterReconfigurationList& reconfigurations
)
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this]() {
        dispatch_reconfigurations_performed();
    };

    m_thread = JThread::JThread(
        [reconfigurations, dispatch_err, dispatch_sta, dispatch_suc](JThread::StopToken stop_token) {
            PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);

            PresetUpdater::perform_reconfigurations(reconfigurations, &process_status);
            if (process_status.has_error()) {
                dispatch_err(process_status.get_error());
                return;
            }
            dispatch_suc();
        }
    );
}

void PresetUpdaterInteractor::update_repositories(const SharedPresetUpdaterRepositoryInfoVector& repos)
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos) {
        dispatch_repository_info_vector(repos);
    };

    m_thread = JThread::JThread([dispatch_err, dispatch_sta, dispatch_suc, repos](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterReconfigurationList reconfigurations;
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);
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

        dispatch_suc(repos);
    });
}

void PresetUpdaterInteractor::add_local_repository(const boost::filesystem::path& zip_path)
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos) {
        dispatch_repository_info_vector(repos);
    };

    m_thread = JThread::JThread([dispatch_err, dispatch_sta, dispatch_suc, zip_path](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterReconfigurationList reconfigurations;
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);
        PresetUpdaterRepositoryDatabase repo_database(&process_status);

        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }

        repo_database.add_local_repository(zip_path, &process_status);
        if (process_status.has_error()) {
            dispatch_err(process_status.get_error());
            return;
        }
        if (stop_token.stop_requested()) {
            return;
        }

        const SharedPresetUpdaterRepositoryInfoVector& repos = repo_database.get_all_repositories();

        dispatch_suc(repos);
    });
}

void PresetUpdaterInteractor::remove_local_repository(const std::string& uuid)
{
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }

    auto dispatch_err = [this](const std::string& body) {
        dispatch_error(body);
    };
    auto dispatch_sta = [this](const std::string& target, int attempt, unsigned delay) {
        dispatch_status(target, attempt, delay);
    };
    auto dispatch_suc = [this](const SharedPresetUpdaterRepositoryInfoVector& repos) {
        dispatch_repository_info_vector(repos);
    };

    m_thread = JThread::JThread([dispatch_err, dispatch_sta, dispatch_suc, uuid](
                                    JThread::StopToken stop_token
                                ) {
        PresetUpdaterReconfigurationList reconfigurations;
        PresetUpdaterProcessStatus process_status(stop_token, dispatch_sta);
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

        dispatch_suc(repos);
    });
}

void PresetUpdaterInteractor::dispatch_error(const std::string& body)
{
    m_dispatcher.dispatch_on_main_thread([this, body]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([body](auto* listener) {
            listener->on_preset_updater_error(body);
        });
    });
}

void PresetUpdaterInteractor::dispatch_reconfigurations_list(
    const PresetUpdaterReconfigurationList& reconfigurations
)
{
    m_dispatcher.dispatch_on_main_thread([this, reconfigurations]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([reconfigurations](auto* listener) {
            listener->on_preset_updater_reconfigurations_list(reconfigurations);
        });
    });
}

void PresetUpdaterInteractor::dispatch_reconfigurations_performed()
{
    m_dispatcher.dispatch_on_main_thread([this]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([](auto* listener) {
            listener->on_preset_updater_reconfigurations_perfomed();
        });
    });
}

void PresetUpdaterInteractor::dispatch_status(const std::string& target, int attempt, unsigned delay)
{
    m_dispatcher.dispatch_on_main_thread([this, target, attempt, delay]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([target, attempt, delay](auto* listener) {
            listener->on_preset_updater_status(target, attempt, delay);
        });
    });
}

void PresetUpdaterInteractor::dispatch_repository_info_vector(
    const SharedPresetUpdaterRepositoryInfoVector& repos
)
{
    m_dispatcher.dispatch_on_main_thread([this, repos]() {
        this->invoke_listeners<IPresetUpdaterResultListener>([repos](auto* listener) {
            listener->on_preset_updater_repository_info_vector(repos);
        });
    });
}

} // namespace Slic3r::Biz::PresetUpdater
