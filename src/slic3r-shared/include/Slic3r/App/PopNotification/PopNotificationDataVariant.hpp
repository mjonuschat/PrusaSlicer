#pragma once

#include "Slic3r/Biz/Platform/JobManager/ProgressTracker.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/RemovableDrive/IRemovableDriveStatusListener.hpp"

#include <variant>
#include <boost/filesystem/path.hpp>

namespace Slic3r::App::PopNotification {

struct DefaultNotificationData
{};

struct JobProgressNotificationData
{
    std::string job_name;
    Biz::Platform::JobManager::Progress progress;
};

struct SlicingProgressNotificationData
{
    Biz::Slicing::Status status;
    Domain::SlicingId slicing_id;
};

enum class PrintHostJobStatus
{
    None,
    Started,
    Finished,
    Failed
};

struct PrintHostProgressNotificationData
{
    size_t print_host_id;
    PrintHostJobStatus status{PrintHostJobStatus::None};
    int progress{0};
    std::string filename;
    std::string target;
    bool is_upload{true};
    std::string additional_msg;
    std::function<void(const boost::filesystem::path&)> eject_fn{nullptr};
};

struct EjectNotificationData
{
    boost::filesystem::path drive_path;
    Biz::RemovableDrive::RemovableDriveStatus status;
};

// Define the variant type alias.
using PopNotificationDataVariant = std::variant<
    DefaultNotificationData,
    JobProgressNotificationData,
    SlicingProgressNotificationData,
    PrintHostProgressNotificationData,
    EjectNotificationData>;

} // namespace Slic3r::App::PopNotification
