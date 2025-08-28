#pragma once

#include "Slic3r/App/PopNotification/PopNotificationDataVariant.hpp"
#include "Slic3r/App/PopNotification/PopNotificationLayout.hpp"

#include "Slic3r/Biz/Platform/PlatformServices.hpp"

#include <string>

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

class PopNotificationData
{
public:
    explicit PopNotificationData(size_t id, PopNotificationType type, PopNotificationLevel level, size_t duration, PopNotificationLayout layout, PopNotificationLayoutVariant layout_variant, PopNotificationDataVariant additional_data = DefaultNotificationData()) :
        m_id(id),
        m_type(type),
        m_level(level),
        m_timeout(duration),
        m_layout(layout),
        m_layout_variant(layout_variant),
        m_additional_data(std::move(additional_data))
    {}

    ~PopNotificationData() = default;

    PopNotificationData(const PopNotificationData&)            = default;
    PopNotificationData(PopNotificationData&&)                 = default;
    PopNotificationData& operator=(const PopNotificationData&) = default;
    PopNotificationData& operator=(PopNotificationData&&)      = default;

    int id() const
    {
        return m_id;
    }

    PopNotificationType type() const
    {
        return m_type;
    }

    PopNotificationLevel level() const
    {
        return m_level;
    }

    void set_level(PopNotificationLevel level)
    {
        m_level = level;
    }

    size_t timeout() const
    {
        return m_timeout;
    }

    void set_timeout(long long timeout)
    {
        m_timeout = timeout;
    }

    Biz::Platform::TimerQueue::TimerID timer_id() const
    {
        return m_timer_id;
    }

    void set_timer_id(Biz::Platform::TimerQueue::TimerID timer_id)
    {
        m_timer_id = timer_id;
    }

    const PopNotificationDataVariant& additional_data() const
    {
        return m_additional_data;
    }

    PopNotificationDataVariant& additional_data()
    {
        return m_additional_data;
    }

    void set_layout(PopNotificationLayout layout, PopNotificationLayoutVariant&& variant)
    {
        m_layout         = layout;
        m_layout_variant = std::move(variant);
    }

    PopNotificationLayout layout() const
    {
        return m_layout;
    }

    const PopNotificationLayoutVariant& layout_variant() const
    {
        return m_layout_variant;
    }

private:
    size_t m_id;
    PopNotificationType m_type;
    PopNotificationLevel m_level;
    size_t m_timeout;
    Biz::Platform::TimerQueue::TimerID m_timer_id;
    PopNotificationDataVariant m_additional_data; // data that needs to be stored
    PopNotificationLayout m_layout;
    PopNotificationLayoutVariant m_layout_variant; // data that needs to be shown by View
};

using PopNotificationDataPtr = std::unique_ptr<PopNotificationData>;

} // namespace Slic3r::App::PopNotification
