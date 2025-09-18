#pragma once

#include "Slic3r/App/PopNotification/PopNotificationDataVariant.hpp"
#include "Slic3r/App/PopNotification/PopNotificationLayout.hpp"
#include "Slic3r/Biz/Platform/TimerQueue.hpp"

namespace Slic3r::App::PopNotification {

enum class PopNotificationType
{
    Custom,
    JobProgress,
    SlicingProgress,
    PrintHostProgress,
    Eject,
    UserAccountLogin,
    UserAccountTransientError,
};

enum class PopNotificationLevel : int
{
    Regular = 1,
    Important,
    Warning,
    Error,
};

struct PopNotificationData
{
    PopNotificationType type;
    PopNotificationLevel level;
    std::chrono::seconds timeout;
    PopNotificationLayout layout; // data that needs to be shown by View
    PopNotificationPayload payload; // data that needs to be stored
    Biz::Platform::TimerQueue::TimerID timer_id;
};

using PopNotificationDataPtr = std::unique_ptr<PopNotificationData>;

} // namespace Slic3r::App::PopNotification
