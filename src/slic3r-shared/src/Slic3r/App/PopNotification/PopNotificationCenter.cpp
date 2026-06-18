#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"
#include "Slic3r/Biz/FileDownloader/FileDownloaderJob.hpp"
#include "Slic3r/App/DisplayStrings.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <ranges>
#include <boost/filesystem/operations.hpp>

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
    m_job_status_functions["printhost"] = std::bind(&PopNotificationCenter::on_job_print_host, this, std::placeholders::_1, std::placeholders::_2);
    m_job_status_functions["file_download"] = std::bind(&PopNotificationCenter::on_download_job_status_changed, this, std::placeholders::_1, std::placeholders::_2);
    // TRN job name to be used in job_status_to_string
    m_job_status_functions["arrange"] = std::bind(&PopNotificationCenter::on_arrange_job_status_changed, this, _u8L("Arrange"), std::placeholders::_2);
    // TRN job name to be used in job_status_to_string
    m_job_status_functions["SimplifyJob"] = std::bind(&PopNotificationCenter::on_job_manager_status_changed_generic, this, _u8L("Simplify"), std::placeholders::_2);

    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor.add_listener<Biz::IProjectsChangedListener>(this);
    m_project_interactor.arrange_interactor().add_listener<Biz::IArrangeEventsListener>(this);
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
    std::string status_text;
    switch (status) {
    case JobStatus::Finished:
        // TRN job_status_to_string, {} is job name.
        return fmt::format(fmt::runtime(_u8L("{} has finished.")), job_name);
    case JobStatus::Failed:
        status_text = _u8L("has failed");
        // TRN job_status_to_string, {} is job name.
        return fmt::format(fmt::runtime(_u8L("{} has failed.")), job_name);
    case JobStatus::Started:
         // TRN job_status_to_string, {} is job name.
        return fmt::format(fmt::runtime(_u8L("{} has started.")), job_name);
    case JobStatus::None:
    default:
        break;
    }

    return {};
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
    // Every new job, that should be displayed 
    for (const auto& [job_name, progress] : status) {
        ASSERT(!job_name.empty());

        [[maybe_unused]] bool found = false;
        for (const auto& pair : m_job_status_functions) {
            if (job_name.starts_with(pair.first)) {
                found = true;
                pair.second(job_name, progress);
                break;
            }
        }

#ifndef NDEBUG
        if (!found) {
            on_job_manager_status_changed_generic("DEBUG ONLY: " + job_name, progress);
        }
#endif
    }
}

void PopNotificationCenter::on_arrange_job_status_changed(const std::string& job_name, const Biz::Platform::JobManager::Progress& progress)
{
    if (progress.status == Domain::JobStatus::Started
        && progress.percent == std::nullopt)
    {
        m_notification_list.erase_notification_by_predicate(
            [](const PopNotificationData& data)
            { return data.type == PopNotificationType::ArrangeEvent; }
        );
    }

    on_job_manager_status_changed_generic(job_name, progress);
}

void PopNotificationCenter::on_job_manager_status_changed_generic(const std::string& job_name, const Biz::Platform::JobManager::Progress& progress)
{
    const std::string text{job_status_to_string(progress.status, job_name)};

    if (text.empty()) {
        return;
    }

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
    size_t total_files;
    if (const auto* payload =
            std::any_cast<FileDownloaderJobProgressPayload>(&progress.progress_detail.payload))
    {
        download_id = payload->download_id;
        filename    = payload->filename;
        dest_path   = payload->final_path;
        printables_url = payload->project_url;
        is_loaded = payload->load_count > 0;
        total_files = payload->total_files;
    } else {
        // Ignore progress without payload
        return;
    }

    bool print_single_file_name = (total_files == 1 || progress.status == JobStatus::Started || progress.status == JobStatus::None);
    std::string text_name = print_single_file_name ?
        // TRN text to be passed to job_status_to_string as job name, {} is filename
        fmt::format(fmt::runtime(_u8L("Downloading {}")), filename) :
        // TRN text to be passed to job_status_to_string as job name, {} is number of files (2 or more)
        fmt::format(fmt::runtime(_u8L("Downloading {} files")), total_files); 
    std::string text        = job_status_to_string(progress.status, text_name);
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
    if (status.code != SlicingStatusCode::Running) {
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

    if (m_project_interactor.workbench().find_project_by_id(slicing_id.project_id) == nullptr) {
        // The project may have been deleted before the queued callback was executed.
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

    if (m_project_interactor.workbench().find_project_by_id(slicing_id.project_id) == nullptr) {
        // The project may have been deleted before the queued callback was executed.
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

    if (m_project_interactor.workbench().find_project_by_id(slicing_id.project_id) == nullptr) {
        // The project may have been deleted before the queued callback was executed.
        return;
    }

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
    using Biz::Slicing::WarningSeverity;
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

    if (m_project_interactor.workbench().find_project_by_id(slicing_id.project_id) == nullptr) {
        // The project may have been deleted before the queued callback was executed.
        return;
    }

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
                warning.severity == WarningSeverity::LOW ? PopNotificationLevel::Warning : PopNotificationLevel::Error,
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

PopNotificationLayout storage_resolve_layout(const PrintHostProgressNotificationData& data)
{
    switch (data.status) {
    case PrintHostJobStatus::None: {
        return PopNotificationLayoutText(_u8L("Resolving PrusaLink storage."));
    }
    case PrintHostJobStatus::Started: {
        return PopNotificationLayoutTextProgress(_u8L("Resolving PrusaLink storage."), data.progress);
    }
    case PrintHostJobStatus::Finished: {
        return PopNotificationLayoutText(_u8L("Resolving storage has finished."));
    }
    case PrintHostJobStatus::Failed: {
        std::string translatable_part = _u8L("Resolving storage has Failed.");
        std::string msg = fmt::format("{} {}", translatable_part, data.additional_msg);       
        return PopNotificationLayoutText(std::move(msg));
    }
    default:
        ASSERT(false);
    }
    return PopNotificationLayoutText("");
}

PopNotificationLayout export_layout(const PrintHostProgressNotificationData& data)
{
    std::string header;
    // Fallback to filename if target path is empty, otherwise show the target path
    std::string body = data.target.empty() ? data.filename : data.target;

    switch (data.status) {
    case PrintHostJobStatus::None: 
        header = _u8L("Export Starting");
        return PopNotificationLayoutHeaderText(header, body);
        
    case PrintHostJobStatus::Started: 
        header = _u8L("Exporting...");
        return PopNotificationLayoutHeaderTextProgress(header, body, data.progress);
        
    case PrintHostJobStatus::Finished: {
        header = _u8L("Export Finished");
        
        if (data.target.empty()) {
            return PopNotificationLayoutHeaderText(header, body);
        }

        std::vector<PopNotificationButtonData> buttons;
        buttons.push_back({_u8L("Open folder"), [data]() {
            boost::filesystem::path target_path(data.target);
            ASSERT(!target_path.empty() && target_path.has_parent_path());
            AppServices::instance().file_explorer_handler().open_folder(
                target_path.parent_path().string()
            );
            return false;
        }});

        if (data.eject_fn != nullptr) {
            buttons.push_back({_u8L("Eject"), [data]() {
                data.eject_fn(data.target);
                return true;
            }});
        }

        return PopNotificationLayoutHeaderTextButtons(header, body, std::move(buttons));
    }
    case PrintHostJobStatus::Failed: 
        header = _u8L("Export Failed");
        body = body.empty() ? data.additional_msg : fmt::format("{}\n{}", body, data.additional_msg);
        return PopNotificationLayoutHeaderText(header, body);
        
    default:
        ASSERT(false);
        return PopNotificationLayoutText("");
    }
}

PopNotificationLayout print_host_layout(const PrintHostProgressNotificationData& data)
{
    if (data.is_storage_resolve) {
        return storage_resolve_layout(data);
    }
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
    if (payload.is_storage_resolve)
    {
        // close notification
        m_notification_list.erase_notification_by_predicate(
            [print_host_id](const PopNotificationData& notification)
            {
                const auto payload{
                    std::get_if<PrintHostProgressNotificationData>(&notification.payload)
                };
                return payload ? payload->print_host_id == print_host_id : false;
            }
        );
        return;
    }
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
            simple ? 10s : 60s,
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
                { 
                    m_removable_drive_service.eject_drive(path); 
                };
        }
    }
    if (tag == PrintHostJobInfoTag::OperationType && msg == "export") {
        payload.is_upload = false;
    }
    if (tag == PrintHostJobInfoTag::OperationType && msg == "storage") {
        payload.is_storage_resolve = true;
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

const auto user_account_login_matcher{cmp<UserAccountLoginNotificationData>( //
    [](const UserAccountLoginNotificationData&, const UserAccountLoginNotificationData&)
    { return true; }
)};

void PopNotificationCenter::on_user_account_id_success(bool is_refresh, const std::string& username)
{
    if (is_refresh) {
        return;
    }
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountLogin);
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    const boost::filesystem::path avatar{m_project_interactor.user_account_interactor().avatar()};
    const std::string image_path{boost::filesystem::exists(avatar) ? avatar.string() : std::string{}};

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Regular,
            10s,
            PopNotificationLayoutImageHeader(image_path,
                fmt::format(fmt::runtime(_u8L("Logged in as {}")), username)),
            UserAccountLoginNotificationData{}
        },
        user_account_login_matcher
    );
}

void PopNotificationCenter::on_avatar_downloaded()
{
    auto& user_account = m_project_interactor.user_account_interactor();
    if (!user_account.is_logged_in()) {
        return;
    }
    // Only update an already shown login notification; avoid popping a new one when the
    // avatar is re-downloaded outside of a fresh login (e.g. during a token refresh).
    std::function<bool(const UserAccountLoginNotificationData&)> any =
        [](const UserAccountLoginNotificationData&) { return true; };
    if (!m_notification_list.get_notifcation_payload<UserAccountLoginNotificationData>(any)) {
        return;
    }

    const boost::filesystem::path avatar{user_account.avatar()};
    if (!boost::filesystem::exists(avatar)) {
        return;
    }

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Regular,
            10s,
            PopNotificationLayoutImageHeader(avatar.string(),
                fmt::format(fmt::runtime(_u8L("Logged in as {}")), user_account.username())),
            UserAccountLoginNotificationData{}
        },
        user_account_login_matcher
    );
}

void PopNotificationCenter::on_user_account_logged_out()
{
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountLogin);
    m_notification_list.close_notifications_of_type(PopNotificationType::UserAccountTransientError);

    const boost::filesystem::path avatar{m_project_interactor.user_account_interactor().avatar()};
    const std::string image_path{boost::filesystem::exists(avatar) ? avatar.string() : std::string{}};

    m_notification_list.upsert_notifcation(
        PopNotificationData{
            PopNotificationType::UserAccountLogin,
            PopNotificationLevel::Regular,
            10s,
            PopNotificationLayoutImageHeader(image_path, _u8L("Successfully logged out")),
            UserAccountLoginNotificationData{}
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

std::optional<int>
get_instance_index(const Domain::ModelObject& model_object, const Domain::SelectionId instance_id)
{
    for (int index = 0; index < int(model_object.instances.size()); ++index) {
        const Domain::ModelInstance* model_instance{model_object.instances[index]};
        ASSERT(model_instance);
        if (model_instance->id().id == instance_id) {
            return index;
        }
    }
    return std::nullopt;
}

void PopNotificationCenter::on_elements_not_arranged(
    Domain::SelectionId project_id,
    const Domain::ElementRefs& elements
)
{
    std::map<std::string, std::vector<int>> instances;
    for (const Domain::ElementRef& element_ref : elements) {
        ASSERT(element_ref.has_instance());
        const Domain::Project& project{m_project_interactor.workbench().project(project_id)};
        const Domain::ModelObject* object{project.find_object_by_id(element_ref.object_id)};
        if (!object) {
            continue;
        }
        const std::optional<int> instance_index{
            get_instance_index(*object, element_ref.instance_id)
        };
        if (!instance_index) {
            instances[object->name];
        } else {
            instances[object->name].push_back(*instance_index);
        }
    }

    const std::string header{_u8L("Some objects could not be arranged")};

    std::string text;
    for (const auto& [name, instance_indicies] : instances) {
        text += name + "\n";
        if (instance_indicies.size() > 1) {
            for (int instance_index : instance_indicies) {
                text += "  "+_u8L("Instance")+ " " + std::to_string(instance_index + 1) + "\n";
            }
        }
    }
    ASSERT(!text.empty());
    ASSERT(text.back() == '\n');
    text.pop_back();

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::ArrangeEvent,
            PopNotificationLevel::Warning,
            0s,
            PopNotificationLayoutHeaderText{header, text},
            ArrangeEventType::ElementsNotArranged,
        },
        [](const PopNotificationPayload& a, const PopNotificationPayload& b)
        {
            const auto a_event_type{std::get_if<ArrangeEventType>(&a)};
            const auto b_event_type{std::get_if<ArrangeEventType>(&b)};
            if (a_event_type == nullptr || b_event_type == nullptr) {
                return false;
            }
            return *a_event_type == *b_event_type;
        }
    );
}

void PopNotificationCenter::on_fatal_arrange_error(
    Domain::SelectionId project_id
)
{
    const std::string header{_u8L("Fatal arrange error")};
    const std::string text{
        _u8L("Arrange failed, this is usually caused by an object being larger than slicer limits.")
    };

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::ArrangeEvent,
            PopNotificationLevel::Error,
            0s,
            PopNotificationLayoutHeaderText{header, text},
            ArrangeEventType::FatalError,
        },
        [](const PopNotificationPayload& a, const PopNotificationPayload& b)
        {
            const auto a_event_type{std::get_if<ArrangeEventType>(&a)};
            const auto b_event_type{std::get_if<ArrangeEventType>(&b)};
            if (a_event_type == nullptr || b_event_type == nullptr) {
                return false;
            }
            return *a_event_type == *b_event_type;
        }
    );
}

void PopNotificationCenter::on_connect_handler_error(const std::string& msg) 
{
    const std::string header{_u8L("Prusa Connect Error")};

    upsert_notifcation(
        PopNotificationData{
            PopNotificationType::ConnectError,
            PopNotificationLevel::Error,
            300s,
            PopNotificationLayoutHeaderText{header, msg}
        },
        never_equal_matcher
    );
}

} // namespace Slic3r::App::PopNotification
