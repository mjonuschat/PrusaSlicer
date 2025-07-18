#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"

#include "Slic3r/App/AppServices.hpp"

#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Log.hpp"

#include <ranges>

using Slic3r::Biz::Platform::JobManager::JobManagerStatus;
using Slic3r::Domain::JobStatus;
using SlicingStatusCode = Slic3r::Biz::Slicing::StatusCode;
using Slic3r::Domain::SlicingId;

namespace Slic3r::App::PopNotification {

void PopNotificationObservableList::add_notification(PopNotificationDataPtr&& notification)
{
    if (notification->timeout() > 0) {
        auto& timer_queue    = Biz::Platform::PlatformServices::instance().timer_queue();
        int duration_milisec = notification->timeout() * 1'000;
        notification->set_timer_id(timer_queue.set_timer(
            std::chrono::milliseconds(duration_milisec),
            std::bind(&PopNotificationObservableList::on_notification_timer, this, notification->id()),
            false
        ));
    }

    m_notifications.emplace_back(std::move(notification));

    invoke_listeners<Biz::IListObserver<PopNotificationData>>(
        [&](auto* l)
        {
            const size_t index = m_notifications.size() - 1;
            l->on_inserted(at(index), index);
        }
    );
}

void PopNotificationObservableList::on_notification_timer(size_t id)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [id](const PopNotificationDataPtr& notification) { return notification->id() == id; }
    );
    erase_notification_by_id(id);
}

void PopNotificationObservableList::erase_notification_by_id(size_t id)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [id](const PopNotificationDataPtr& notification) { return notification->id() == id; }
    );
    erase_notification(it);
}

PopNotificationDataIt PopNotificationObservableList::erase_notification(PopNotificationDataIt it)
{
    ASSERT(it != m_notifications.end());
    stop_notification_timer(it);
    const size_t index = std::distance(m_notifications.begin(), it);
    it                 = m_notifications.erase(it);

    invoke_listeners<Biz::IListObserver<PopNotificationData>>([&](auto* l)
                                                              { l->on_removed({index}); });
    return it;
}

void PopNotificationObservableList::notification_updated(PopNotificationDataIt it)
{
    ASSERT(it != m_notifications.end());
    const size_t index = std::distance(m_notifications.begin(), it);
    invoke_listeners<Biz::IListObserver<PopNotificationData>>([&](auto* l)
                                                              { l->on_updated({index}); });
}

void PopNotificationObservableList::set_notification_timeout(PopNotificationDataIt it, size_t seconds)
{
    ASSERT(it != m_notifications.end());
    stop_notification_timer(it);
    if (seconds == 0) {
        return;
    }
    it->get()->set_timeout(seconds);
    auto& timer_queue    = Biz::Platform::PlatformServices::instance().timer_queue();
    int duration_milisec = seconds * 1'000;
    it->get()->set_timer_id(timer_queue.set_timer(
        std::chrono::milliseconds(duration_milisec),
        std::bind(&PopNotificationObservableList::on_notification_timer, this, it->get()->id()),
        false
    ));
}

void PopNotificationObservableList::stop_notification_timer(PopNotificationDataIt it)
{
    ASSERT(it != m_notifications.end());
    auto& timer_queue = Biz::Platform::PlatformServices::instance().timer_queue();
    if (it->get()->timer_id() != Biz::Platform::TimerQueue::TimerID()
        && timer_queue.is_timer_running(it->get()->timer_id()))
    {
        timer_queue.cancel_timer(it->get()->timer_id());
    }
    it->get()->set_timeout(0);
}

void PopNotificationObservableList::on_notification_close_button(size_t id)
{
    SPDLOG_INFO("{} id:{}", __FUNCTION__, id);
    erase_notification_by_id(id);
}

void PopNotificationObservableList::on_notification_hover(size_t id) {}

const PopNotificationData& PopNotificationObservableList::at(size_t index) const
{
    return *m_notifications[index].get();
}

size_t PopNotificationObservableList::size() const
{
    return m_notifications.size();
}

void PopNotificationObservableList::close_notifications_of_type(PopNotificationType type)
{
    for (auto it = m_notifications.begin(); it != m_notifications.end();) {
        if ((*it)->type() == type) {
            it = erase_notification(it);
        } else {
            ++it;
        }
    }
}

namespace {
std::string job_status_to_string(const JobStatus status)
{
    switch (status) {
    case JobStatus::None:
        return "None";
    case JobStatus::Started:
        return "Started";
    case JobStatus::Finished:
        return "Finished";
    case JobStatus::Failed:
        return "Failed";
    default:
        return "Unknown";
    }
}
} // namespace

void PopNotificationCenter::on_job_manager_status_changed(const JobManagerStatus& status)
{
    for (const auto& [job_name, progress] : status) {
        ASSERT(!job_name.empty());
        std::string text = fmt::format("{}: {}", job_name, job_status_to_string(progress.status));

        auto it = std::find_if(
            m_notifications.begin(),
            m_notifications.end(),
            [&job_name](const PopNotificationDataPtr& notif_ptr)
            {
                if (notif_ptr->type() != PopNotificationType::JobProgress) {
                    return false;
                }
                const auto* job_data = std::get_if<JobProgressNotificationData>(
                    &notif_ptr->additional_data()
                );
                return job_data && job_data->job_name == job_name;
            }
        );

        if (it != m_notifications.end()) {
            PopNotificationDataPtr& notif_ptr = *it;
            auto* job_data = std::get_if<JobProgressNotificationData>(&notif_ptr->additional_data());
            if (job_data->progress.status != JobStatus::Finished
                && progress.status == JobStatus::Finished)
            {
                set_notification_timeout(it, 5);
            }
            job_data->progress = progress;

            if (progress.percent) {
                int perc = (int) (progress.percent.value().value * 100);
                notif_ptr->set_layout(
                    PopNotificationLayout::TextProgress,
                    PopNotificationLayoutTextProgress(text, perc)
                );
            } else {
                notif_ptr->set_layout(PopNotificationLayout::Text, PopNotificationLayoutText(text));
            }

            notification_updated(it);
        } else {
            add_notification(PopNotificationFactory::create_job_notification(text, job_name, progress));
        }
    }
}

namespace {
std::string slicing_status_to_string(const SlicingStatusCode status)
{
    switch (status) {
    case SlicingStatusCode::Empty:
        return "Empty";
    case SlicingStatusCode::Updating:
        return "Updating";
    case SlicingStatusCode::Running:
        return "Running";
    case SlicingStatusCode::Finished:
        return "Finished";
    case SlicingStatusCode::Modified:
        return "Modified";
    case SlicingStatusCode::Stopping:
        return "Stopping";
    case SlicingStatusCode::Removed:
        return "Removed";
    default:
        return "Unknown";
    }
}
} // namespace

void PopNotificationCenter::on_status_changed(const Biz::Slicing::Status status, const SlicingId slicing_id)
{
    if (status.code != SlicingStatusCode::Running
        && status.code != SlicingStatusCode::Finished
        && status.code != SlicingStatusCode::Stopping)
    {
        return;
    }
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [&slicing_id](const PopNotificationDataPtr& notif_ptr)
        { return notif_ptr->type() == PopNotificationType::SlicingProgress; }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<SlicingProgressNotificationData>(&(*it)->additional_data());
        job_data->slicing_id = slicing_id;

        if (job_data->status.code != status.code && status.code == SlicingStatusCode::Finished) {
            set_notification_timeout(it, 5);
        }
        job_data->status = status;

        (*it)->set_layout(PopNotificationLayout::HeaderText, PopNotificationLayoutHeaderText("Slicing", slicing_status_to_string(status.code)));
        notification_updated(it);
    } else {
        add_notification(PopNotificationFactory::create_slicing_notification(slicing_status_to_string(status.code), status, slicing_id));
    }
}

namespace {
/*
enum class  PrintHostJobStatus
{
    None,
    Started,
    Finished,
    Failed
};
*/

PopNotificationLayoutVariant upload_layout(
    const PrintHostProgressNotificationData& data,
    PopNotificationLayout& ret_type
)
{
    switch (data.status) {
    case PrintHostJobStatus::None: {
        ret_type = PopNotificationLayout::Text;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = "Upload is starting.";
        } else if (data.target.empty()) {
            msg = fmt::format("Upload of {} is starting.", data.filename);
        } else if (data.filename.empty()) {
            msg = fmt::format("Upload to {} is starting.", data.target);
        } else {
            msg = fmt::format("Upload of {} to {} is starting.", data.filename, data.target);
        }
        return PopNotificationLayoutText(std::move(msg));
    }
    case PrintHostJobStatus::Started: {
        ret_type = PopNotificationLayout::TextProgress;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = "Uploading.";
        } else if (data.target.empty()) {
            msg = fmt::format("Uploading {}.", data.filename);
        } else if (data.filename.empty()) {
            msg = fmt::format("Uploading to {}.", data.target);
        } else {
            msg = fmt::format("Uploading {} to {}.", data.filename, data.target);
        }
        return PopNotificationLayoutTextProgress(std::move(msg), data.progress);
    }
    case PrintHostJobStatus::Finished: {
        ret_type = PopNotificationLayout::Text;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = "Uploading has Finished.";
        } else if (data.target.empty()) {
            msg = fmt::format("Uploading {} has Finished.", data.filename);
        } else if (data.filename.empty()) {
            msg = fmt::format("Uploading to {} has Finished.", data.target);
        } else {
            msg = fmt::format("Uploading {} to {} has Finished.", data.filename, data.target);
        }
        return PopNotificationLayoutText(std::move(msg));
    }
    case PrintHostJobStatus::Failed: {
        ret_type = PopNotificationLayout::Text;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = fmt::format("Uploading has Failed. {}", data.additional_msg);
        } else if (data.target.empty()) {
            msg = fmt::format("Uploading {} has Failed. {}", data.filename, data.additional_msg);
        } else if (data.filename.empty()) {
            msg = fmt::format("Uploading to {} has Failed. {}", data.target, data.additional_msg);
        } else {
            msg = fmt::format(
                "Uploading {} to {} has Failed. {}",
                data.filename,
                data.target,
                data.additional_msg
            );
        }
        return PopNotificationLayoutText(std::move(msg));
    }
    default:
        ASSERT(false);
    }
    return PopNotificationLayoutText("");
}

PopNotificationLayoutVariant export_layout(
    const PrintHostProgressNotificationData& data,
    PopNotificationLayout& ret_type
)
{
    switch (data.status) {
    case PrintHostJobStatus::None: {
        ret_type = PopNotificationLayout::Text;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = "Export is starting.";
        } else if (data.target.empty()) {
            msg = fmt::format("Export of {} is starting.", data.filename);
        } else {
            msg = fmt::format("Export to {} is starting.", data.target);
        }
        return PopNotificationLayoutText(std::move(msg));
    }
    case PrintHostJobStatus::Started: {
        ret_type = PopNotificationLayout::TextProgress;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = "Exporting.";
        } else if (data.target.empty()) {
            msg = fmt::format("Exporting {}.", data.filename);
        } else {
            msg = fmt::format("Exporting to {}.", data.target);
        }
        return PopNotificationLayoutTextProgress(std::move(msg), data.progress);
    }
    case PrintHostJobStatus::Finished: {
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg      = "Exporting has Finished.";
            ret_type = PopNotificationLayout::Text;
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.target.empty()) {
            msg      = fmt::format("Exporting {} has Finished.", data.filename);
            ret_type = PopNotificationLayout::Text;
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.eject_fn == nullptr) {
            msg      = fmt::format("Exporting to {} has Finished.", data.target);
            ret_type = PopNotificationLayout::TextButtons;
            return PopNotificationLayoutTextButtons(
                std::move(msg),
                {{"Open folder",
                  [data]()
                  {
                      boost::filesystem::path target_path(data.target);
                      ASSERT(!target_path.empty() && target_path.has_parent_path());
                      AppServices::instance().file_explorer_handler().open_folder(
                          target_path.parent_path().string()
                      );
                      return true;
                  }}}
            );
        } else {
            msg      = fmt::format("Exporting to {} has Finished.", data.target);
            ret_type = PopNotificationLayout::TextButtons;
            return PopNotificationLayoutTextButtons(
                std::move(msg),
                {{"Open folder",
                  [data]()
                  {
                      boost::filesystem::path target_path(data.target);
                      ASSERT(!target_path.empty() && target_path.has_parent_path());
                      AppServices::instance().file_explorer_handler().open_folder(
                          target_path.parent_path().string()
                      );
                      return true;
                  }},
                 {"eject",
                  [data]()
                  {
                      data.eject_fn(data.target);
                      return true;
                  }}}
            );
        }
    }
    case PrintHostJobStatus::Failed: {
        ret_type = PopNotificationLayout::Text;
        std::string msg;
        if (data.filename.empty() && data.target.empty()) {
            msg = fmt::format("Exporting has Failed. {}", data.additional_msg);
        } else if (data.target.empty()) {
            msg = fmt::format("Exporting {} has Failed. {}", data.filename, data.additional_msg);
        } else {
            msg = fmt::format("Exporting to {} has Failed. {}", data.target, data.additional_msg);
        }
        return PopNotificationLayoutText(std::move(msg));
    }
    default:
        ASSERT(false);
    }
    return PopNotificationLayoutText("");
}

PopNotificationLayoutVariant print_host_layout(
    const PrintHostProgressNotificationData& data,
    PopNotificationLayout& ret_type
)
{
    if (data.is_upload) {
        return upload_layout(data, ret_type);
    } else {
        return export_layout(data, ret_type);
    }
}

} // namespace

void PopNotificationCenter::on_print_host_progress(size_t print_host_id, int progress)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::PrintHostProgress) {
                return false;
            }
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(
                &notif_ptr->additional_data()
            );
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<PrintHostProgressNotificationData>(&(*it)->additional_data());
        job_data->status   = PrintHostJobStatus::Started;
        job_data->progress = progress;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(*job_data, layout_type);
        (*it)->set_layout(layout_type, std::move(layout_variant));
        notification_updated(it);
    } else {
        PrintHostProgressNotificationData data(print_host_id);
        data.status   = PrintHostJobStatus::Started;
        data.progress = progress;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(data, layout_type);
        add_notification(
            std::make_unique<PopNotificationData>(
                PopNotificationFactory::next_id(),
                PopNotificationType::PrintHostProgress,
                PopNotificationLevel::Important,
                0,
                layout_type,
                std::move(layout_variant),
                std::move(data)
            )
        );
    }
}

void PopNotificationCenter::on_print_host_error(size_t print_host_id, const std::string& msg)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::PrintHostProgress) {
                return false;
            }
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(
                &notif_ptr->additional_data()
            );
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<PrintHostProgressNotificationData>(&(*it)->additional_data());
        job_data->status         = PrintHostJobStatus::Failed;
        job_data->progress       = -1;
        job_data->additional_msg = msg;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(*job_data, layout_type);
        (*it)->set_layout(layout_type, std::move(layout_variant));
        (*it)->set_level(PopNotificationLevel::Error);
        notification_updated(it);
    } else {
        PrintHostProgressNotificationData data(print_host_id);
        data.status         = PrintHostJobStatus::Failed;
        data.progress       = -1;
        data.additional_msg = msg;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(data, layout_type);
        add_notification(
            std::make_unique<PopNotificationData>(
                PopNotificationFactory::next_id(),
                PopNotificationType::PrintHostProgress,
                PopNotificationLevel::Error,
                0,
                layout_type,
                std::move(layout_variant),
                std::move(data)
            )
        );
    }
}

void PopNotificationCenter::on_print_host_cancel(size_t print_host_id)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::PrintHostProgress) {
                return false;
            }
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(
                &notif_ptr->additional_data()
            );
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        erase_notification(it);
    }
}

void PopNotificationCenter::on_print_host_done(size_t print_host_id)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::PrintHostProgress) {
                return false;
            }
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(
                &notif_ptr->additional_data()
            );
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<PrintHostProgressNotificationData>(&(*it)->additional_data());
        job_data->status   = PrintHostJobStatus::Finished;
        job_data->progress = 100;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(*job_data, layout_type);
        (*it)->set_layout(layout_type, std::move(layout_variant));
        notification_updated(it);
    } else {
        PrintHostProgressNotificationData data(print_host_id);
        data.status   = PrintHostJobStatus::Finished;
        data.progress = 100;
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(data, layout_type);
        add_notification(
            std::make_unique<PopNotificationData>(
                PopNotificationFactory::next_id(),
                PopNotificationType::PrintHostProgress,
                PopNotificationLevel::Important,
                0,
                layout_type,
                std::move(layout_variant),
                std::move(data)
            )
        );
    }
}

void PopNotificationCenter::on_print_host_info(
    size_t print_host_id,
    const std::string& tag,
    const std::string& msg
)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::PrintHostProgress) {
                return false;
            }
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(
                &notif_ptr->additional_data()
            );
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<PrintHostProgressNotificationData>(&(*it)->additional_data());
        if (tag == "filename") {
            job_data->filename = msg;
        }
        if (tag == "resolve") {
            job_data->target = msg;
            if (m_removable_drive_service.is_path_on_removable_drive(boost::filesystem::path(msg))) {
                job_data->eject_fn = [this](const boost::filesystem::path& path)
                { m_removable_drive_service.eject_drive(path); };
            }
        }
        if (tag == "is_export") {
            job_data->is_upload = false;
        }
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(*job_data, layout_type);
        (*it)->set_layout(layout_type, std::move(layout_variant));
        notification_updated(it);
    } else {
        PrintHostProgressNotificationData data(print_host_id);
        if (tag == "filename") {
            data.filename = msg;
        }
        if (tag == "resolve") {
            data.target = msg;
            if (m_removable_drive_service.is_path_on_removable_drive(boost::filesystem::path(msg))) {
                data.eject_fn = [this](const boost::filesystem::path& path)
                { m_removable_drive_service.eject_drive(path); };
            }
        }
        if (tag == "is_export") {
            data.is_upload = false;
        }
        PopNotificationLayout layout_type;
        auto layout_variant = print_host_layout(data, layout_type);
        add_notification(
            std::make_unique<PopNotificationData>(
                PopNotificationFactory::next_id(),
                PopNotificationType::PrintHostProgress,
                PopNotificationLevel::Important,
                0,
                layout_type,
                std::move(layout_variant),
                std::move(data)
            )
        );
    }
}

namespace {
std::string removable_drive_status_to_string(
    const boost::filesystem::path& drive_path,
    Biz::RemovableDrive::RemovableDriveStatus status
)
{
    switch (status) {
    case Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Inserted:
        return {};
    case Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Ejecting:
        return fmt::format("Ejecting {}.", drive_path.string());

    case Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Removed:
        return fmt::format("Ejecting {} done.", drive_path.string());
        break;
    case Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Failed:
        return fmt::format("Ejecting {} has failed.", drive_path.string());
        break;
    default:
        ASSERT(false, "Missing status handling.");
    }
    return {};
}
} // namespace

void PopNotificationCenter::on_removable_drive_status_changed(
    const boost::filesystem::path& drive_path,
    Biz::RemovableDrive::RemovableDriveStatus status
)
{
    // Ignore inserted state.
    if (status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Inserted) {
        return;
    }
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [drive_path](const PopNotificationDataPtr& notif_ptr)
        {
            if (notif_ptr->type() != PopNotificationType::Eject) {
                return false;
            }
            auto* job_data = std::get_if<EjectNotificationData>(&notif_ptr->additional_data());
            return job_data && job_data->drive_path == drive_path;
        }
    );

    if (it != m_notifications.end()) {
        auto* job_data = std::get_if<EjectNotificationData>(&(*it)->additional_data());
        ASSERT(job_data);

        if (job_data->status != status) {
            job_data->status = status;
            if (status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Ejecting) {
                set_notification_timeout(it, 0);
                (*it)->set_level(PopNotificationLevel::Regular);
            } else if (status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Removed) {
                set_notification_timeout(it, 10);
                (*it)->set_level(PopNotificationLevel::Regular);
            } else if (status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Failed) {
                set_notification_timeout(it, 10);
                (*it)->set_level(PopNotificationLevel::Warning);
            }
            (*it)->set_layout(
                PopNotificationLayout::Text,
                PopNotificationLayoutText(removable_drive_status_to_string(drive_path, status))
            );
            notification_updated(it);
        }
        // Ignore Removed status if there is now previous ejecting (its not being ejected by us)
    } else if (status != Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Removed) {
        add_notification(
            std::make_unique<PopNotificationData>(
                PopNotificationFactory::next_id(),
                PopNotificationType::Eject,
                status != Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Failed ?
                    PopNotificationLevel::Regular :
                    PopNotificationLevel::Warning,
                status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Ejecting ? 0 : 10,
                PopNotificationLayout::Text,
                PopNotificationLayoutText(removable_drive_status_to_string(drive_path, status)),
                EjectNotificationData(drive_path, status)
            )
        );
    }
}

void PopNotificationCenter::on_user_account_id_success(bool is_refresh, const std::string& username)
{
    if (is_refresh) {
        return;
    }
    close_notifications_of_type(PopNotificationType::UserAccountLogin);
    close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    add_notification(
        std::make_unique<PopNotificationData>(
            PopNotificationFactory::next_id(),
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Important,
            10,
            PopNotificationLayout::Text,
            PopNotificationLayoutText(fmt::format("User {} logged in.", username)),
            DefaultNotificationData()
        )
    );
}

void PopNotificationCenter::on_user_account_logged_out()
{
    close_notifications_of_type(PopNotificationType::UserAccountLogin);
    close_notifications_of_type(PopNotificationType::UserAccountTransientError);
    add_notification(
        std::make_unique<PopNotificationData>(
            PopNotificationFactory::next_id(),
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Important,
            10,
            PopNotificationLayout::Text,
            PopNotificationLayoutText("User Account logged out."),
            DefaultNotificationData()
        )
    );
}

void PopNotificationCenter::on_user_account_will_refresh()
{ /*unused*/
}

void PopNotificationCenter::on_user_account_action_retry(
    const Biz::Network::IHttp::Retry& retry,
    std::function<void(void)> cancel_callback
)
{
    ASSERT(cancel_callback);
    close_notifications_of_type(PopNotificationType::UserAccountLogin);
    close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    std::string text = fmt::format(
        "(Attempt {}) Communication with Prusa Account is taking longer than expected. Retrying. Attempt {}.",
        std::to_string(retry.attempt),
        std::to_string(retry.attempt)
    );
    add_notification(
        std::make_unique<PopNotificationData>(
            PopNotificationFactory::next_id(),
            PopNotificationType::UserAccountTransientError,
            PopNotificationLevel::Warning,
            0,
            PopNotificationLayout::TextButtons,
            PopNotificationLayoutTextButtons(
                text,
                {{"Cancel",
                  [cancel_callback]()
                  {
                      cancel_callback();
                      return true;
                  }}}
            ),
            DefaultNotificationData()
        )
    );
}

} // namespace Slic3r::App::PopNotification
