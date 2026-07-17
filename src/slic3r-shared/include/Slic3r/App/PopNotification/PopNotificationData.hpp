#pragma once

#include "Slic3r/App/PopNotification/PopNotificationDataVariant.hpp"
#include "Slic3r/App/PopNotification/PopNotificationLayout.hpp"
#include "Slic3r/Biz/Platform/TimerQueue.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::App::PopNotification {

enum class PopNotificationType
{
    Custom,
    JobProgress,
    SlicingProgress,
    PrintHostProgress,
    DownloadProgress,
    Eject,
    SimplifySuggestion,
    UserAccountLogin,
    UserAccountTransientError,
    SlicingWarning,
    SlicingError,
    LoadError,
    ArrangeEvent,
    ConnectError,
    FileExplorerError
};

/*
 * @brief Tells how notification should look in UI
 */
enum class PopNotificationLevel : int
{
    Regular = 1, // Standart colors, closing button
    ProgressNoClose, // Standart colors, no closing button
    ProgressWithClose,
    Warning, // orange colors, left picture
    Error, // red colors, left picture
};

struct PopNotificationData
{
    PopNotificationType type;
    PopNotificationLevel level;
    std::chrono::seconds timeout;
    PopNotificationLayout layout; // data that needs to be shown by View
    PopNotificationPayload payload; // data that needs to be stored
    Domain::SelectionId project_id{Domain::INVALID_ID};
    Biz::Platform::TimerQueue::TimerID timer_id;
};

using PopNotificationDataPtr = std::unique_ptr<PopNotificationData>;

} // namespace Slic3r::App::PopNotification
