#include "PresetUpdaterWrapper.hpp"

#include "PresetArchiveRepository.hpp"
#include "PresetUpdaterProcessStatus.hpp"
#include "PresetUpdaterReconfigurations.hpp"

#include "../../Utils/Format.hpp"
#include "../../Utils/Utils.hpp"

#include <boost/log/trivial.hpp>


using namespace std::chrono;

namespace PresetManagement {

namespace {
void log_reconfigurations(const ReconfigurationsList& reconfigurations)
{
    BOOST_LOG_TRIVIAL(info) << "Reconfigurations: updates: " << reconfigurations.regular_updates().size() << " forced updates: " << reconfigurations.forced_updates().size() << " downgrades: " << reconfigurations.forced_downgrades().size();
    for (const auto& reconf :reconfigurations.regular_updates()) {
        BOOST_LOG_TRIVIAL(info) << "update: " << reconf.vendor_id;
    }
    for (const auto& reconf :reconfigurations.forced_updates()) {
        BOOST_LOG_TRIVIAL(info) << "forced update: " << reconf.vendor_id;
    }
    for (const auto& reconf :reconfigurations.forced_downgrades()) {
        BOOST_LOG_TRIVIAL(info) << "forced downgrade: " << reconf.vendor_id;
    }
}
} // namespace

 PresetUpdaterWrapper::~PresetUpdaterWrapper()
 {
     cancel_worker_thread();
 }
void PresetUpdaterWrapper::cancel_worker_thread_and_reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy policy)
{
    assert(m_process_status);
    cancel_worker_thread();
    m_process_status->reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_NO_RETRY);
}
void PresetUpdaterWrapper::cancel_worker_thread()
{
     if (m_worker_thread.joinable()) {
        m_process_status->force_cancel();
		m_worker_thread.join();
	}
}

void PresetUpdaterWrapper::check_forced_reconfigurations(ErrorFn error_fn, ReconfigurationsCalculatedFn reconf_fn, ReconfigurationsPerformedFn success_fn)
{
    cancel_worker_thread_and_reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_NO_RETRY);

    // TODO: rethink this
    // Seemingly useless worker thread just to unify approach to members.
    std::string error_msg;
    auto worker_body = [this, error_fn, reconf_fn, success_fn]()
    {
        ReconfigurationsList reconfigurations;
        m_preset_updater.check_forced_reconfigurations(reconfigurations, m_process_status.get());
        if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
        if (m_process_status->get_canceled()) { return; }

        log_reconfigurations(reconfigurations);
        if (reconf_fn(reconfigurations)) {
            m_preset_updater.perform_reconfigurations(reconfigurations, m_process_status.get());
            if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
            if (m_process_status->get_canceled()) { return; }
        }
        success_fn();
    }; 
    m_worker_thread = std::thread(worker_body);
}
  
// typedef std::function<void(std::string /* message */)> ErrorFn;
// typedef std::function<void(const ReconfigurationsList& /* reconfigurations */)> ReconfigurationsCalculatedFn;
void PresetUpdaterWrapper::build_update_sync_and_reconfiguration_check(ErrorFn error_fn, ReconfigurationsCalculatedFn reconf_fn, ReconfigurationsPerformedFn success_fn)
{
    cancel_worker_thread_and_reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_NO_RETRY);
    
    auto worker_body = [this, error_fn, reconf_fn, success_fn]()
    {
        m_preset_archive_repo_database.sync(m_process_status.get()); 
        if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
        if (m_process_status->get_canceled()) { return; }

        const SharedArchiveRepositoryVector &repos = m_preset_archive_repo_database.get_selected_archive_repositories();
        m_preset_archive_sync.sync(repos, m_process_status.get());
        if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
        if (m_process_status->get_canceled()) { return; }

        ReconfigurationsList reconfigurations;
        m_preset_updater.check_reconfigurations(reconfigurations, m_process_status.get());
        if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
        if (m_process_status->get_canceled()) { return; }

        log_reconfigurations(reconfigurations);
        if (reconf_fn(reconfigurations)) {
            m_preset_updater.perform_reconfigurations(reconfigurations, m_process_status.get());
            if (m_process_status->has_error()) { error_fn(m_process_status->get_error()); }
            if (m_process_status->get_canceled()) { return; }
        }
        success_fn();
    }; 
    m_worker_thread = std::thread(worker_body);
}

}

