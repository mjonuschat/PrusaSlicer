#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDescriptor.hpp"
#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"

#include <jthread/JThread.hpp>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterInteractor : public WithListeners<IPresetUpdaterResultListener>
{
public:
    typedef std::function<void(std::string /* message */)> ErrorFn;
    typedef std::function<bool(const PresetUpdaterReconfigurationList& /* reconfigurations */)>
        ReconfigurationsCalculatedFn;
    typedef std::function<void(void)> ReconfigurationsPerformedFn;

    PresetUpdaterInteractor(Platform::IMainThreadDispatcher& dispatcher);
    ~PresetUpdaterInteractor();

    /**
     * @brief Only checks existing installed files against app version. Success is dispatch_reconfigurations_list.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     */
    void check_forced_reconfigurations();

    /**
     * @brief Does full construction of update_sync folder and checks reconfigurations. Success is dispatch_reconfigurations_list.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     */
    void build_update_sync_and_reconfiguration_check(bool ignore_hash = false);

    /**
     * @brief Performs all reconfigurations in ReconfigurationList. Success is dispatch_reconfigurations_performed.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     */
    void perform_reconfigurations(const PresetUpdaterReconfigurationList& reconfigurations);

    /**
     * @brief Updates selection of source repositories. Success is dispatch_repository_info_vector.
     * Repository selection is stored in app manifest file.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     */
    void update_repositories(const SharedPresetUpdaterRepositoryInfoVector& descriptor);

    /**
     * @brief Adds local repository to app manifest file. Success is dispatch_repository_info_vector.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     * @param unselect_others means other repos with same id will be unselected. Otherwise more than one repo with same id might be selected.
     */
    void add_local_repository(const boost::filesystem::path& zip_path, bool unselect_others);

    /**
     * @brief Removes local repository to app manifest file. Success is dispatch_repository_info_vector.
     * All public methods of PresetUpdaterInteractor runs in worker thread (max 1 in time).
     * Results are dispatched to IPresetUpdaterResultListener.
     * None of the resources are shared. All objects needed for preset management are created only inside the worker thread.
     */
    void remove_local_repository(const std::string& uuid);

    void list_repositories();

    void cleanup_update_sync();

private:
    JThread::JThread m_thread;
    Platform::IMainThreadDispatcher& m_dispatcher;

    // Error - operation has failed
    void dispatch_error(const std::string& body);
    // Status - operation is ongoing
    void dispatch_status(const std::string& target, int attempt, unsigned delay);
    // Success - operation has finished
    void dispatch_reconfigurations_list(const PresetUpdaterReconfigurationList& reconfigurations, const std::vector<PresetUpdaterWarning>& warnings);
    void dispatch_reconfigurations_performed(const std::vector<PresetUpdaterWarning>& warnings);
    void dispatch_repository_info_vector(const SharedPresetUpdaterRepositoryInfoVector& descriptor, const std::vector<PresetUpdaterWarning>& warnings);
};

} // namespace Slic3r::Biz::PresetUpdater
