#ifndef slic3r_PresetUpdateWrapper_hpp_
#define slic3r_PresetUpdateWrapper_hpp_

#include "PresetArchiveRepositoryDatabase.hpp"
#include "PresetArchiveSync.hpp"
#include "PresetUpdater.hpp"
#include "PresetUpdaterProcessStatus.hpp"

#include "../../Utils/Http.hpp"

#include <memory>
#include <functional>
#include <thread>

namespace Slic3r { class Semver; }
namespace PresetManagement {

class PresetUpdaterProcessStatus;
class ReconfigurationsList;

class PresetUpdaterWrapper
{
public:
    typedef std::function<void(std::string /* message */)> ErrorFn;
    typedef std::function<bool(const ReconfigurationsList& /* reconfigurations */)> ReconfigurationsCalculatedFn;
    typedef std::function<void(void)> ReconfigurationsPerformedFn;

    PresetUpdaterWrapper()
        : m_preset_archive_repo_database()
        , m_preset_updater()
        , m_preset_archive_sync()
        , m_process_status(std::make_unique<PresetUpdaterProcessStatus>()){}
    PresetUpdaterWrapper(PresetUpdaterWrapper&&) = delete;
    PresetUpdaterWrapper(const PresetUpdaterWrapper&) = delete;
    PresetUpdaterWrapper& operator=(PresetUpdaterWrapper&&) = delete;
    PresetUpdaterWrapper& operator=(const PresetUpdaterWrapper&) = delete;
    ~PresetUpdaterWrapper();

    /// Only checks existing installed files against app version
    void check_forced_reconfigurations(ErrorFn error_fn, ReconfigurationsCalculatedFn reconf_fn, ReconfigurationsPerformedFn success_fn);
 
    /// Does full construction of update_sync folder and checks reconfigurations
    /// Might be triggered with blocking UI (f.e. loading dialog) or fully background 
    void build_update_sync_and_reconfiguration_check(ErrorFn error_fn, ReconfigurationsCalculatedFn reconf_fn, ReconfigurationsPerformedFn success_fn);

private:
    // Member objects. These do all the work. Should be called only from methods of this class.
    PresetArchiveRepositoryDatabase             m_preset_archive_repo_database;
    PresetUpdater                               m_preset_updater;
    PresetArchiveSync                           m_preset_archive_sync;

    void cancel_worker_thread_and_reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy policy);
    void cancel_worker_thread();
    std::unique_ptr<PresetUpdaterProcessStatus> m_process_status;
    std::thread m_worker_thread;
};

} // namespace PresetManagement 
#endif //slic3r_PresetUpdateWrapper_hpp_