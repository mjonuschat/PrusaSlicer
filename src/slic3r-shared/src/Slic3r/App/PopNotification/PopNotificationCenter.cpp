#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"
#include "Slic3r/Biz/FileDownloader/FileDownloaderJob.hpp"
#include "Slic3r/App/DisplayStrings.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <ranges>

using Slic3r::Biz::Platform::JobManager::JobManagerStatus;
using Slic3r::Biz::Platform::JobManager::Progress;
using Slic3r::Domain::JobStatus;
using SlicingStatusCode = Slic3r::Biz::Slicing::StatusCode;
using Slic3r::Biz::PrintHost::PrintHostJobProgressPayload;
using Slic3r::Biz::PrintHost::PrintHostJobProgressState;
using Slic3r::Biz::PrintHost::PrintHostJobInfoTag;
using Slic3r::Biz::FileDownloader::FileDownloaderJobProgressPayload;
using Slic3r::Domain::SlicingId;

using namespace Slic3r::Biz;

namespace Slic3r::App::PopNotification {

PopNotificationCenter::PopNotificationCenter(Biz::ProjectInteractor& project_interactor) :
    m_removable_drive_service(project_interactor.removable_drive_service()),
    m_project_interactor(project_interactor)
{
    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor.add_listener<Biz::IProjectsChangedListener>(this);
    m_list_sort_filter.set_source_model(&m_notification_list);

    auto sort_fn = [](const PopNotificationData& lhs, const PopNotificationData& rhs)
    {
        if (lhs.level == PopNotificationLevel::ProgressNoClose || lhs.level == PopNotificationLevel::ProgressWithClose) {
            return false;
        }
        if (rhs.level == PopNotificationLevel::ProgressNoClose || rhs.level == PopNotificationLevel::ProgressWithClose) {
            return true;
        }
        return false;
    };
    m_list_sort_filter.set_sort_fn(sort_fn);
}

void PopNotificationCenter::upsert_notifcation(PopNotificationData data, PopNotificationObservableList::Matcher matcher)
{
    m_notification_list.upsert_notifcation(std::move(data), matcher);
}


namespace {
std::string job_status_to_string(const JobStatus& status, const std::string& job_name)
{
    std::string ui_name = job_name;
    if (job_name == "arrange") {
        ui_name = _u8L("Arrange");
    }

    std::string status_text;
    switch (status) {
    case JobStatus::Finished:
        status_text = _u8L("finished");
        break;
    case JobStatus::Failed:
        status_text = _u8L("failed");
        break;
    case JobStatus::Started:
    case JobStatus::None:
    default:
        break;
    }

    return fmt::format("{} {}", ui_name, status_text);
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

        if (job_name.starts_with("printhost")) {
            on_job_print_host(job_name, progress);
            continue;
        }

        if (job_name.starts_with("file_download")) {
            on_download_job_status_changed(job_name, progress);
            continue;
        }

        // Follows generic job notification logic. 
        const std::string text{job_status_to_string(progress.status, job_name)};

        PopNotificationLayout layout;
        if (progress.status == JobStatus::Started && progress.percent) {
            int perc = (int) (progress.percent.value().value * 100);
            layout   = PopNotificationLayoutTextProgress(text, perc);
        } else {
            layout = PopNotificationLayoutText(text);
        }

        PopNotificationData notification{
            PopNotificationType::JobProgress,
            progress.status == JobStatus::Started ? PopNotificationLevel::ProgressNoClose :
                                                    PopNotificationLevel::ProgressWithClose,
            progress.status == JobStatus::Started ? 0s : 5s,
            layout,
            JobProgressNotificationData(job_name, progress)
        };

        using Payload = JobProgressNotificationData;
        const auto matcher{cmp<Payload>( //
            [](const Payload& a, const Payload& b) { return a.job_name == b.job_name; }
        )};
        m_notification_list.upsert_notifcation(std::move(notification), matcher);
    }
}

void PopNotificationCenter::on_job_print_host(const std::string& string, const Progress& progress)
{

    size_t payload_id;
    PrintHostJobInfoTag payload_tag;
    std::string payload_message;
    if (const auto* payload = std::any_cast<PrintHostJobProgressPayload>(&progress.progress_detail.payload))
    {
        payload_id = payload->id;
        payload_tag = payload->tag;
        payload_message = payload->message;

    } else {
        // Ignore all progress without payload. It should be only first started status.
        ASSERT(progress.status == JobStatus::Started);
        return;
    }

    switch ((PrintHostJobProgressState) progress.progress_detail.info) {
    case PrintHostJobProgressState::None:
        if (progress.status == JobStatus::Finished)
        {
            on_print_host_done(payload_id);
        } else {
            if (progress.percent)
            {
                on_print_host_progress(payload_id, (int) (progress.percent.value().value * 100));
            }   
        }
        
        break;
    case PrintHostJobProgressState::Info: {
        on_print_host_info(payload_id, payload_tag, payload_message);
    } break;
    case PrintHostJobProgressState::Error: {
        on_print_host_error(payload_id, payload_message);
    } break;
    default:
        ASSERT(false, "Missing PrintHostJobProgressState handling.");
        break;
    }
    
}

const auto download_job_matcher{cmp<DownloadProgressNotificationData>( //
    [](const DownloadProgressNotificationData& a, const DownloadProgressNotificationData& b)
    { return a.download_id == b.download_id; }
)};

void PopNotificationCenter::on_download_job_status_changed(
    const std::string& string,
    const Biz::Platform::JobManager::Progress& progress
)
{
    size_t download_id;
    std::string filename;
    boost::filesystem::path dest_path;
    std::string printables_url;
    bool is_loaded;
    if (const auto* payload =
            std::any_cast<FileDownloaderJobProgressPayload>(&progress.progress_detail.payload))
    {
        download_id = payload->download_id;
        filename    = payload->filename;
        dest_path   = payload->final_path;
        printables_url = payload->project_url;
        is_loaded = payload->load_count > 0;
    } else {
        // Ignore progress without payload
        return;
    }

    std::string text = job_status_to_string(progress.status, fmt::format("Downloading {}", filename));
    PopNotificationLayout layout;
    PopNotificationLevel level = PopNotificationLevel::ProgressWithClose;
    if (progress.status == JobStatus::Finished) {
        if (!is_loaded) {
            layout = PopNotificationLayoutTextButtons{
                text,
                {{_u8L("Load"),
                  [this, dest_path]()
                  {
                      m_project_interactor.open_downloaded_file(dest_path, false);
                      return true;
                  }},
                 {_u8L("Load as New Project"),
                  [this, dest_path]()
                  {
                      m_project_interactor.open_downloaded_file(dest_path, true);
                      return true;
                  }}}
            };
        } else {
            std::vector<PopNotificationButtonData> buttons;
            if (!printables_url.empty()) {
                buttons.emplace_back(
                    PopNotificationButtonData{
                        _u8L("Printables"),
                        [this, printables_url]()
                        {
                            if (m_switch_left_tab_fn) {
                                m_switch_left_tab_fn(LeftBarTabs::Printables, printables_url);
                            }
                            return true;
                        }
                    }
                );
            }
            if (!dest_path.empty()) {
                buttons.emplace_back(
                    PopNotificationButtonData{
                        _u8L("Open Folder"),
                        [this, dest_path]()
                        {
                            ASSERT(!dest_path.empty() && dest_path.has_parent_path());
                            AppServices::instance().file_explorer_handler().open_folder(
                                dest_path.parent_path().string()
                            );
                            return true;
                        }
                    }
                );
            }
            if (buttons.empty()) {
                layout = PopNotificationLayoutText{text};
            } else {
                layout = PopNotificationLayoutTextButtons{text, std::move(buttons)};
            } 
        }
        
    } else if (progress.status == JobStatus::Finished) {
        layout = PopNotificationLayoutText{text};
    } else if (progress.percent) {
        int perc = (int) (progress.percent.value().value * 100);
        layout   = PopNotificationLayoutTextProgress(text, perc);
        level    = PopNotificationLevel::ProgressNoClose;
    } else {
        layout = PopNotificationLayoutText(text);
    }

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::DownloadProgress,
            level,
            progress.status == JobStatus::Finished ? 20s : 0s,
            std::move(layout),
            DownloadProgressNotificationData(download_id)
        },
        download_job_matcher
    );
}

namespace {
std::string slicing_status_to_string(const SlicingStatusCode status)
{
    switch (status) {
    case SlicingStatusCode::Empty:
        return _u8L("Empty");
    case SlicingStatusCode::Updating:
        return _u8L("Updating");
    case SlicingStatusCode::Running:
        return _u8L("Slicing");
    case SlicingStatusCode::Finished:
        return _u8L("Slicing Finished");
    case SlicingStatusCode::Modified:
        return _u8L("Modified");
    case SlicingStatusCode::Stopping:
        return _u8L("Slicing Stopped");
    case SlicingStatusCode::Removed:
        return _u8L("Removed");
    case SlicingStatusCode::InvalidData:
        return _u8L("Invalid settings");
    default:
        return "Unknown";
    }
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
        m_notification_list.erase_notification_by_predicate(
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

    const std::string header{m_project_interactor.get_project_name(slicing_id.project_id)};
    const std::string text{slicing_status_to_string(status.code)};

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::SlicingProgress,
            status.code == SlicingStatusCode::Running ? PopNotificationLevel::ProgressNoClose : PopNotificationLevel::ProgressWithClose,
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

    const std::string header{m_project_interactor.get_project_name(slicing_id.project_id)};
    const std::string text{to_display_string(status.progress->progress_info)};

    const int progress{static_cast<int>(std::round(status.progress->progress.value))};
    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::SlicingProgress,
            status.code == SlicingStatusCode::Running ? PopNotificationLevel::ProgressNoClose : PopNotificationLevel::ProgressWithClose,
            0s,
            PopNotificationLayoutHeaderTextProgress{header, text, progress},
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
    m_notification_list.erase_notification_by_predicate(
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
    const std::string header{m_project_interactor.get_project_name(slicing_id.project_id)};
    const Domain::Project& project{m_project_interactor.workbench().project(slicing_id.project_id)};
    for (const Error& error : errors) {
        m_notification_list.upsert_notifcation(
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
    m_notification_list.erase_notification_by_predicate(
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
    const std::string header{m_project_interactor.get_project_name(slicing_id.project_id)};
    const Domain::Project& project{m_project_interactor.workbench().project(slicing_id.project_id)};
    for (const Warning& warning : warnings) {
        m_notification_list.upsert_notifcation(
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
            msg = "Uploading has finished.";
        } else if (data.target.empty()) {
            msg = fmt::format("Uploading {} has finished.", data.filename);
        } else if (data.filename.empty()) {
            msg = fmt::format("Uploading to {} has finished.", data.target);
        } else {
            msg = fmt::format("Uploading {} to {} has finished.", data.filename, data.target);
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
            msg = "Exporting has finished.";
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.target.empty()) {
            msg = fmt::format("Exporting {} has finished.", data.filename);
            return PopNotificationLayoutText(std::move(msg));
        } else if (data.eject_fn == nullptr) {
            msg = fmt::format("Exporting to {} has finished.", data.target);
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
            msg = fmt::format("Exporting to {} has finished.", data.target);
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
    const Payload* previous_payload{m_notification_list.get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status   = PrintHostJobStatus::Started;
    payload.progress = progress;
    auto layout{print_host_layout(payload)};

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::ProgressNoClose,
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
    const Payload* previous_payload{m_notification_list.get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status         = PrintHostJobStatus::Failed;
    payload.progress       = -1;
    payload.additional_msg = msg;
    auto layout            = print_host_layout(payload);
    m_notification_list.upsert_notifcation(
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
    m_notification_list.erase_notification_by_predicate(
            [print_host_id](const PopNotificationData& notification)
            {
                const auto payload{std::get_if<PrintHostProgressNotificationData>(&notification.payload)};
                if (payload == nullptr) {
                    return false;
                }
                return payload->print_host_id == print_host_id;
            }
        );
}

void PopNotificationCenter::on_print_host_done(size_t print_host_id)
{
    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{m_notification_list.get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    payload.status   = PrintHostJobStatus::Finished;
    payload.progress = 100;
    auto layout      = print_host_layout(payload);
    bool simple = false;
    if (std::holds_alternative<PopNotificationLayoutText>(layout)) {
        simple = true;
    }
    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            PopNotificationLevel::ProgressWithClose,
            simple ? 10s : 0s,
            std::move(layout),
            std::move(payload)
        },
        print_host_matcher
    );
}

void PopNotificationCenter::on_print_host_info(
    size_t print_host_id,
    const PrintHostJobInfoTag& tag,
    const std::string& msg
)
{
    using Payload = PrintHostProgressNotificationData;
    const Payload* previous_payload{m_notification_list.get_notifcation_payload<Payload>(
        [=](const Payload& payload) { return payload.print_host_id == print_host_id; }
    )};
    Payload payload{previous_payload == nullptr ? Payload{print_host_id} : *previous_payload};
    if (tag == PrintHostJobInfoTag::Filename) {
        payload.filename = msg;
    }
    if (tag == PrintHostJobInfoTag::Resolve) {
        payload.target = msg;
        if (m_removable_drive_service.is_path_on_removable_drive(boost::filesystem::path(msg))) {
            payload.eject_fn = [this](const boost::filesystem::path& path)
            { m_removable_drive_service.eject_drive(path); };
        }
    }
    if (tag == PrintHostJobInfoTag::OperationType && msg == "export") { // todo: also msg "storage"
        payload.is_upload = false;
    }
    auto layout = print_host_layout(payload);
    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::PrintHostProgress,
            payload.status == PrintHostJobStatus::Started ? PopNotificationLevel::ProgressNoClose : PopNotificationLevel::ProgressWithClose,
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
    m_notification_list.upsert_notifcation(
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

void PopNotificationCenter::on_user_account_id_success(bool is_refresh, const std::string& username)
{
    if (is_refresh) {
        return;
    }
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountLogin);
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Regular,
            10s,
            PopNotificationLayoutText(fmt::format("User {} logged in.", username))
        },
        never_equal_matcher
    );
}

void PopNotificationCenter::on_user_account_logged_out()
{
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountLogin);
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountTransientError);
    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Regular,
            10s,
            PopNotificationLayoutText("User Account logged out.")
        },
        never_equal_matcher
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
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountLogin);
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    std::string text = fmt::format(
        "(Attempt {}) Communication with Prusa Account is taking longer than expected. Retrying. Attempt {}.",
        std::to_string(retry.attempt),
        std::to_string(retry.attempt)
    );
    m_notification_list.upsert_notifcation(
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
        never_equal_matcher
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
        never_equal_matcher
    );
}


} // namespace Slic3r::App::PopNotification
