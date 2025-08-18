#pragma once

#include "Slic3r/App/PopNotification/PopNotificationData.hpp"
#include "Slic3r/App/PopNotification/PopNotificationFactory.hpp"
#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/Platform/JobManager/IJobManagerStatusChangedListener.hpp"
#include "Slic3r/Biz/PrintHost/IPrintHostListener.hpp"
#include "Slic3r/Biz/RemovableDrive/IRemovableDriveStatusListener.hpp"
#include "Slic3r/Biz/RemovableDrive/RemovableDriveService.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"

#include <vector>

namespace Slic3r::App::PopNotification {

using PopNotificationDataIt = std::vector<PopNotificationDataPtr>::iterator;

class PopNotificationObservableList : public Biz::IObservableList<PopNotificationData>
{
public:
    PopNotificationObservableList()  = default;
    ~PopNotificationObservableList() = default;

    PopNotificationObservableList(const PopNotificationObservableList&)            = delete;
    PopNotificationObservableList& operator=(const PopNotificationObservableList&) = delete;
    PopNotificationObservableList(PopNotificationObservableList&&)                 = delete;
    PopNotificationObservableList& operator=(PopNotificationObservableList&&)      = delete;

    void add_notification(PopNotificationDataPtr&& notification);
    void close_notifications_of_type(PopNotificationType type);

    void on_notification_close_button(size_t id);
    void on_notification_hover(size_t id);

    // IObservableList methods
    /**
     * @return const reference to element at index
     */
    const PopNotificationData& at(size_t index) const override;

    /**
     * @return size of items in list
     */
    size_t size() const override;

protected:
    void on_notification_timer(size_t id);
    void erase_notification_by_id(size_t id);
    PopNotificationDataIt erase_notification(PopNotificationDataIt it);
    void notification_updated(PopNotificationDataIt it);
    void set_notification_timeout(PopNotificationDataIt it, size_t seconds);
    void stop_notification_timer(PopNotificationDataIt it);

    std::vector<PopNotificationDataPtr> m_notifications;
};

class PopNotificationCenter :
    public PopNotificationObservableList,
    public Biz::Platform::JobManager::IJobManagerStatusChangedListener,
    public Biz::Slicing::IStatusListener,
    public Biz::PrintHost::IPrintHostListener,
    public Biz::RemovableDrive::IRemovableDriveStatusListener,
    public Biz::UserAccount::IUserAccountListener
{
public:
    PopNotificationCenter(Biz::RemovableDrive::RemovableDriveService& removable_drive_service) :
        m_removable_drive_service(removable_drive_service)
    {}

    ~PopNotificationCenter() = default;

    // Job
    void on_job_manager_status_changed(const Biz::Platform::JobManager::JobManagerStatus& status) override;

    // Slicing
    void on_status_changed(const Biz::Slicing::Status status, const Domain::SlicingId slicing_id) override;

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

private:
    Biz::RemovableDrive::RemovableDriveService& m_removable_drive_service;
};

} // namespace Slic3r::App::PopNotification
