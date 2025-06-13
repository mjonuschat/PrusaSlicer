#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryCredentials.hpp"
#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"

#include <jthread/JThread.hpp>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterInteractor : public WithListeners<IPresetUpdaterResultListener>
{
public:
    typedef std::function<void(std::string /* message */)> ErrorFn;
    typedef std::function<bool(const PresetUpdaterReconfigurationList& /* reconfigurations */)> ReconfigurationsCalculatedFn;
    typedef std::function<void(void)> ReconfigurationsPerformedFn;

    PresetUpdaterInteractor(Platform::IMainThreadDispatcher& dispatcher);
    ~PresetUpdaterInteractor();

    /// Only checks existing installed files against app version
    void check_forced_reconfigurations();
 
    /// Does full construction of update_sync folder and checks reconfigurations
    /// Might be triggered with blocking UI (f.e. loading dialog) or fully background 
    void build_update_sync_and_reconfiguration_check();

    void perform_reconfigurations(const PresetUpdaterReconfigurationList& reconfigurations);

    void update_repositories(const SharedPresetUpdaterRepositoryInfoVector& descriptor);
    void add_local_repository(const boost::filesystem::path& zip_path);
	void remove_local_repository(const std::string& uuid);     
    
private:
    JThread::JThread m_thread;
    Platform::IMainThreadDispatcher& m_dispatcher;

    void dispatch_error(const std::string& body);
    void dispatch_reconfigurations_list(const PresetUpdaterReconfigurationList& reconfigurations);
    void dispatch_reconfigurations_performed();
    void dispatch_status(const std::string& target, int attempt, unsigned delay);
    void dispatch_repository_info_vector(const SharedPresetUpdaterRepositoryInfoVector& descriptor);
};

} // namespace Slic3r::Biz::PresetUpdater