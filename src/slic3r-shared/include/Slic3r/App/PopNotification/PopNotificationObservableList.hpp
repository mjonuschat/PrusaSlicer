#pragma once

#include "Slic3r/App/PopNotification/PopNotificationData.hpp"
#include "Slic3r/App/PopNotification/PopNotificationFactory.hpp"
#include "Slic3r/Biz/IObservableList.hpp"

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
    void erase_notification_by_index(size_t index);
    void notification_updated(size_t index);
    void set_notification_timeout(PopNotificationDataIt it, size_t seconds);
    void stop_notification_timer(PopNotificationDataIt it) const;

    std::vector<PopNotificationDataPtr> m_notifications;
};

} // namespace Slic3r::App::PopNotification
