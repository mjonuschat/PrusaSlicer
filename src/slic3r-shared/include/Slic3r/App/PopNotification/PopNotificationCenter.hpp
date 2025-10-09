#pragma once

#include "Slic3r/App/PopNotification/PopNotificationObservableList.hpp"
#include "Slic3r/Biz/Platform/JobManager/IJobManagerStatusChangedListener.hpp"
#include "Slic3r/Biz/PrintHost/IPrintHostListener.hpp"
#include "Slic3r/Biz/RemovableDrive/IRemovableDriveStatusListener.hpp"
#include "Slic3r/Biz/RemovableDrive/RemovableDriveService.hpp"
#include "Slic3r/Biz/StatusCache.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App::PopNotification {

class PopNotificationCenter :
    public PopNotificationObservableList,
    public Biz::Platform::JobManager::IJobManagerStatusChangedListener,
    public Biz::IStatusCacheChangedListener,
    public Biz::PrintHost::IPrintHostListener,
    public Biz::RemovableDrive::IRemovableDriveStatusListener,
    public Biz::UserAccount::IUserAccountListener,
    public Biz::IProjectsChangedListener
{
public:
    PopNotificationCenter(Biz::ProjectInteractor& project_interactor);

    // Job
    void on_job_manager_status_changed(const Biz::Platform::JobManager::JobManagerStatus& status) override;

    // Slicing
    void on_status_cache_status_code_changed(const Domain::SlicingId slicing_id) override;
    void on_status_cache_progress_changed(const Domain::SlicingId slicing_id) override;
    void on_status_cache_errors_changed(const Domain::SlicingId slicing_id) override;
    void on_status_cache_warnings_changed(const Domain::SlicingId slicing_id) override;

    // Export / Upload
    void on_print_host_progress(size_t id, int progress) override;
    void on_print_host_error(size_t id, const std::string& msg) override;
    void on_print_host_cancel(size_t id) override;
    void on_print_host_done(size_t id) override;
    void on_print_host_info(size_t id, const std::string& tag, const std::string& msg) override;

    // Removable Drive
    void on_removable_drive_status_changed(const boost::filesystem::path& drive_path, Biz::RemovableDrive::RemovableDriveStatus status) override;

    // User account
    void on_user_account_id_success(bool is_refresh, const std::string& username) override;
    void on_user_account_logged_out() override;
    void on_user_account_will_refresh() override;
    void on_user_account_action_retry(const Biz::Network::IHttp::Retry& retry, std::function<void(void)> cancel_callback) override;

    // Projects Changed
    void on_project_load_failed(const std::string& error) override;

private:
    Biz::RemovableDrive::RemovableDriveService& m_removable_drive_service;
    Biz::ProjectInteractor& m_project_interactor;
};

} // namespace Slic3r::App::PopNotification
