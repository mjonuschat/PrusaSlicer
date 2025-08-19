#include "Slic3r/App/PopNotification/PopNotificationObservableList.hpp"
#include "Slic3r/Log.hpp"

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
    int id = notification->id();
    m_notifications.emplace_back(std::move(notification));
    SPDLOG_INFO("invoke_listeners on_inserted {} size {} id {}", std::to_string(m_notifications.size() - 1), m_notifications.size(), id);
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
    int id = it->get()->id();
    it                 = m_notifications.erase(it);
    SPDLOG_INFO("invoke_listeners on_removed {} size {} id {}", std::to_string(index), m_notifications.size(), id);
    invoke_listeners<Biz::IListObserver<PopNotificationData>>(
        [&](auto* l) { l->on_removed({index}); }
    );
    return it;
}

void PopNotificationObservableList::notification_updated(PopNotificationDataIt it)
{
    ASSERT(it != m_notifications.end());
    const size_t index = std::distance(m_notifications.begin(), it);
    SPDLOG_INFO("invoke_listeners on_updated {} size {} id {}", std::to_string(index), m_notifications.size(), it->get()->id());
    invoke_listeners<Biz::IListObserver<PopNotificationData>>(
        [&](auto* l) { l->on_updated({index}); }
    );
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

} // namespace Slic3r::App::PopNotification
