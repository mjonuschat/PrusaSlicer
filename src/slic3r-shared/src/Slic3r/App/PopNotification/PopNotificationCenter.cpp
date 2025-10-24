#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"
#include "Slic3r/App/DisplayStrings.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <ranges>

using Slic3r::Biz::Platform::JobManager::JobManagerStatus;
using Slic3r::Domain::JobStatus;
using SlicingStatusCode = Slic3r::Biz::Slicing::StatusCode;
using Slic3r::Domain::SlicingId;

using namespace Slic3r::Biz;

namespace Slic3r::App::PopNotification {

PopNotificationCenter::PopNotificationCenter(Biz::ProjectInteractor& project_interactor) :
    m_removable_drive_service(project_interactor.removable_drive_service()),
    m_project_interactor(project_interactor)
{
    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor.add_listener<Biz::IProjectsChangedListener>(this);
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

template <typename T>
std::function<bool(const PopNotificationPayload&, const PopNotificationPayload&)> cmp(
    std::function<bool(const T&, const T&)> comparator
)
{
    return [=](const PopNotificationPayload& a, const PopNotificationPayload& b)
    {
        const auto* a_payload{std::get_if<T>(&a)};
        const auto* b_payload{std::get_if<T>(&b)};
        if (a_payload == nullptr || b_payload == nullptr) {
            return false;
        }
        return comparator(*a_payload, *b_payload);
    };
}

void PopNotificationCenter::on_job_manager_status_changed(const JobManagerStatus& status)
{
    for (const auto& [job_name, progress] : status) {
        ASSERT(!job_name.empty());
        const std::string text{
            fmt::format("{}: {}", job_name, job_status_to_string(progress.status))
        };

        PopNotificationLayout layout;
        if (progress.percent) {
            int perc = (int) (progress.percent.value().value * 100);
            layout   = PopNotificationLayoutTextProgress(text, perc);
        } else {
            layout = PopNotificationLayoutText(text);
        }

        PopNotificationData notification{
            PopNotificationType::JobProgress,
            PopNotificationLevel::Important,
            0s,
            layout,
            JobProgressNotificationData(job_name, progress)
        };

        using Payload = JobProgressNotificationData;
        const auto matcher{cmp<Payload>( //
            [](const Payload& a, const Payload& b) { return a.job_name == b.job_name; }
        )};
        upsert_notifcation(std::move(notification), matcher);
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
        return "Stopped";
    case SlicingStatusCode::Removed:
        return "Removed";
    case SlicingStatusCode::InvalidData:
        return "Invalid settings";
    default:
        return "Unknown";
    }
}

std::string get_slicing_header(
    const std::string& prefix,
    const SlicingId slicing_id,
    const Biz::ProjectInteractor& project_interactor
)
{
    return prefix + " " + project_interactor.get_project_name(slicing_id.project_id);
}
} // namespace


const auto slicing_matcher{cmp<SlicingStatusNotificationData>( //
    [](const SlicingStatusNotificationData& a, const SlicingStatusNotificationData& b)
    { return a.slicing_id.project_id == b.slicing_id.project_id; }
)};

void PopNotificationCenter::on_status_cache_status_code_changed(const SlicingId slicing_id)
{
    auto optional_status{m_project_interactor.status_cache().get_status(slicing_id)};
    if (!optional_status) {
        return;
    }
    const Biz::Slicing::Status status{std::move(*optional_status)};
    if (status.code != SlicingStatusCode::Running
        && status.code != SlicingStatusCode::Finished
        && status.code != SlicingStatusCode::Stopping
        && status.code != SlicingStatusCode::InvalidData
        )
    {
        return;
    }

    using Payload = SlicingStatusNotificationData;
    if (status.code == SlicingStatusCode::InvalidData) {
        erase_notification_by_predicate(
            [slicing_id](const PopNotificationData& notification)
            {
                const auto payload{std::get_if<Payload>(&notification.payload)};
                if (payload == nullptr) {
                    return false;
                }
                return payload->slicing_id == slicing_id;
            }
        );
        return;
    }

    const std::string header{get_slicing_header(_u8L("Slicing"), slicing_id, m_project_interactor)};
    const std::string text{slicing_status_to_string(status.code)};

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::SlicingProgress,
            PopNotificationLevel::Important,
            status.code == Biz::Slicing::StatusCode::Finished ? 5s : 0s,
            PopNotificationLayoutHeaderText{header, text},
            SlicingStatusNotificationData{slicing_id}
        },
        slicing_matcher
    );
}

void PopNotificationCenter::on_status_cache_progress_changed(const Domain::SlicingId slicing_id) {
    auto optional_status{m_project_interactor.status_cache().get_status(slicing_id)};
    if (!optional_status) {
        return;
    }
    const Biz::Slicing::Status status{std::move(*optional_status)};
    if (!status.progress) {
        return;
    }
    const std::string header{
        get_slicing_header(_u8L("Slicing in progress: "), slicing_id, m_project_interactor)
        + "\n" + to_display_string(status.progress->progress_info)
    };
    const int progress{static_cast<int>(std::round(status.progress->progress.value))};
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::SlicingProgress,
            PopNotificationLevel::Important,
            0s,
            PopNotificationLayoutTextProgress{header, progress},
            SlicingStatusNotificationData{slicing_id}
        },
        slicing_matcher
    );
}

void PopNotificationCenter::on_status_cache_errors_changed(const Domain::SlicingId slicing_id) {
    using Biz::Slicing::Error;
    using Biz::Slicing::ErrorCode;
    using Payload = SlicingErrorNotificationData;

    auto optional_status{m_project_interactor.status_cache().get_status(slicing_id)};
    if (!optional_status) {
        return;
    }

    std::set<ErrorCode> present_error_codes;
    for (const Error& error : optional_status->errors) {
        present_error_codes.insert(error.code);
    }
    erase_notification_by_predicate(
        [&](const PopNotificationData& notification)
        {
            const auto payload{std::get_if<Payload>(&notification.payload)};
            if (payload == nullptr) {
                return false;
            }
            return payload->slicing_id == slicing_id
                && !present_error_codes.contains(payload->error_code);
        }
    );

    const std::vector<Error> errors{
        m_project_interactor.status_cache().extract_latest_errors(slicing_id)
    };

    const auto matcher{cmp<Payload>( //
        [](const Payload& a, const Payload& b)
        {
            return a.slicing_id.project_id == b.slicing_id.project_id
                && a.error_code == b.error_code;
        }
    )};
    const std::string header{
        get_slicing_header(_u8L("Error: "), slicing_id, m_project_interactor)
    };
    const Domain::Project& project{m_project_interactor.workbench().project(slicing_id.project_id)};
    for (const Error& error : errors) {
        upsert_notifcation(
            PopNotificationData{
                PopNotificationType::SlicingError,
                PopNotificationLevel::Error,
                0s,
                PopNotificationLayoutHeaderText{header, to_display_string(error, project)},
                Payload{error.code, slicing_id}
            },
            matcher
        );
    }
}

void PopNotificationCenter::on_status_cache_warnings_changed(const Domain::SlicingId slicing_id) {
    using Biz::Slicing::Warning;
    using Biz::Slicing::WarningCode;
    using Payload = SlicingWarningNotificationData;

    auto optional_status{m_project_interactor.status_cache().get_status(slicing_id)};
    if (!optional_status) {
        return;
    }

    std::set<WarningCode> present_warning_codes;
    for (const Warning& warning : optional_status->warrnings) {
        present_warning_codes.insert(warning.code);
    }
    erase_notification_by_predicate(
        [&](const PopNotificationData& notification)
        {
            const auto payload{std::get_if<Payload>(&notification.payload)};
            if (payload == nullptr) {
                return false;
            }
            return payload->slicing_id == slicing_id
                && !present_warning_codes.contains(payload->warning_code);
        }
    );

    const std::vector<Warning> warnings{
        m_project_interactor.status_cache().extract_latest_warnings(slicing_id)
    };

    const auto matcher{cmp<Payload>( //
        [](const Payload& a, const Payload& b)
        {
            return a.slicing_id.project_id == b.slicing_id.project_id
                && a.warning_code == b.warning_code;
        }
    )};
    const std::string header{
        get_slicing_header(_u8L("Warning: "), slicing_id, m_project_interactor)
    };
    const Domain::Project& project{m_project_interactor.workbench().project(slicing_id.project_id)};
    for (const Warning& warning : warnings) {
        upsert_notifcation(
            PopNotificationData{
                PopNotificationType::SlicingWarning,
                PopNotificationLevel::Warning,
                0s,
                PopNotificationLayoutHeaderText{header, to_display_string(warning, project)},
                Payload{warning.code, slicing_id}
            },
            matcher
        );
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

PopNotificationLayout upload_layout(const PrintHostProgressNotificationData& data)
{
    switch (data.status) {
    case PrintHostJobStatus::None: {
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

PopNotificationLayout export_layout(const PrintHostProgressNotificationData& data)
{
    switch (data.status) {
    case PrintHostJobStatus::None: {
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
            msg = "Exporting has Finished.";
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.target.empty()) {
            msg = fmt::format("Exporting {} has Finished.", data.filename);
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.eject_fn == nullptr) {
            msg = fmt::format("Exporting to {} has Finished.", data.target);
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
                      return false;
                  }}}
            );
        } else {
            msg = fmt::format("Exporting to {} has Finished.", data.target);
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
                      return false;
                  }},
                 {"Eject",
                  [data]()
                  {
                      data.eject_fn(data.target);
                      return true;
                  }}}
            );
        }
    }
    case PrintHostJobStatus::Failed: {
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

PopNotificationLayout print_host_layout(const PrintHostProgressNotificationData& data)
{
    if (data.is_upload) {
        return upload_layout(data);
    } else {
        return export_layout(data);
    }
}

const auto print_host_matcher{cmp<PrintHostProgressNotificationData>( //
    [](const PrintHostProgressNotificationData& a, const PrintHostProgressNotificationData& b)
    { return a.print_host_id == b.print_host_id; }
)};

} // namespace

void PopNotificationCenter::on_print_host_progress(size_t print_host_id, int progress)
{
    using namespace std::chrono_literals;

    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status   = PrintHostJobStatus::Started;
    payload.progress = progress;
    auto layout{print_host_layout(payload)};

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::Important,
            0s,
            layout,
            std::move(payload)
        },
        print_host_matcher
    );
}

void PopNotificationCenter::on_print_host_error(size_t print_host_id, const std::string& msg)
{
    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status         = PrintHostJobStatus::Failed;
    payload.progress       = -1;
    payload.additional_msg = msg;
    auto layout            = print_host_layout(payload);
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::Error,
            0s,
            std::move(layout),
            std::move(payload)
        },
        print_host_matcher
    );
}

void PopNotificationCenter::on_print_host_cancel(size_t print_host_id)
{
    auto it = std::find_if(
        m_notifications.begin(),
        m_notifications.end(),
        [print_host_id](const PopNotificationDataPtr& notif_ptr)
        {
            auto* job_data = std::get_if<PrintHostProgressNotificationData>(&notif_ptr->payload);
            return job_data && job_data->print_host_id == print_host_id;
        }
    );

    if (it != m_notifications.end()) {
        erase_notification_by_index(std::distance(m_notifications.begin(), it));
    }
}

void PopNotificationCenter::on_print_host_done(size_t print_host_id)
{
    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status   = PrintHostJobStatus::Finished;
    payload.progress = 100;
    auto layout      = print_host_layout(payload);
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::Important,
            0s,
            std::move(layout),
            std::move(payload)
        },
        print_host_matcher
    );
}

void PopNotificationCenter::on_print_host_info(
    size_t print_host_id,
    const std::string& tag,
    const std::string& msg
)
{
    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    if (tag == "filename") {
        payload.filename = msg;
    }
    if (tag == "resolve") {
        payload.target = msg;
        if (m_removable_drive_service.is_path_on_removable_drive(boost::filesystem::path(msg))) {
            payload.eject_fn = [this](const boost::filesystem::path& path)
            { m_removable_drive_service.eject_drive(path); };
        }
    }
    if (tag == "is_export") {
        payload.is_upload = false;
    }
    auto layout = print_host_layout(payload);
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::Important,
            0s,
            std::move(layout),
            std::move(payload)
        },
        print_host_matcher
    );
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

    using Payload = EjectNotificationData;
    const auto matcher{cmp<Payload>( //
        [](const Payload& a, const Payload& b) { return a.drive_path == b.drive_path; }
    )};
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::Eject,
            status != Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Failed ?
                PopNotificationLevel::Regular :
                PopNotificationLevel::Warning,
            status == Slic3r::Biz::RemovableDrive::RemovableDriveStatus::Ejecting ? 0s : 10s,
            PopNotificationLayoutText(removable_drive_status_to_string(drive_path, status)),
            EjectNotificationData(drive_path, status)
        },
        matcher
    );
}

const auto never_equal{[](const PopNotificationPayload&, const PopNotificationPayload&)
                       { return false; }};

void PopNotificationCenter::on_user_account_id_success(bool is_refresh, const std::string& username)
{
    if (is_refresh) {
        return;
    }
    close_notifications_of_type(PopNotificationType::UserAccountLogin);
    close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Important,
            10s,
            PopNotificationLayoutText(fmt::format("User {} logged in.", username))
        },
        never_equal
    );
}

void PopNotificationCenter::on_user_account_logged_out()
{
    close_notifications_of_type(PopNotificationType::UserAccountLogin);
    close_notifications_of_type(PopNotificationType::UserAccountTransientError);
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Important,
            10s,
            PopNotificationLayoutText("User Account logged out.")
        },
        never_equal
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
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountTransientError,
            PopNotificationLevel::Warning,
            0s,
            PopNotificationLayoutTextButtons(
                text,
                {{"Cancel",
                  [cancel_callback]()
                  {
                      cancel_callback();
                      return true;
                  }}}
            )
        },
        never_equal
    );
}

void PopNotificationCenter::on_project_load_failed(const std::string& error)
{
    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::LoadError,
            PopNotificationLevel::Warning,
            0s,
            PopNotificationLayoutText{error}
        },
        never_equal
    );
}


} // namespace Slic3r::App::PopNotification
