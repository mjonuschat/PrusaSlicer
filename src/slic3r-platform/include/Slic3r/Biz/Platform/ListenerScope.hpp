///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

namespace Slic3r::Biz {

/**
 * @brief The ListenerScope class is a RAII utility wrapper for add_listener and remove_listener
 * methods. It is intended to put this class as a class attribute.
 */
template<class Listener, class Publisher, class Observer>
class ListenerScope
{
public:
    ListenerScope(Publisher& publisher, Observer& observer)
        : m_publisher(publisher), m_observer(observer)
    {
        m_publisher.template add_listener<Listener>(&m_observer);
    }
    virtual ~ListenerScope() { m_publisher.template remove_listener<Listener>(&m_observer); }
    ListenerScope(const ListenerScope& rhs) = delete;
    ListenerScope& operator=(const ListenerScope& rhs) = delete;

private:
    Publisher& m_publisher;
    Observer& m_observer;
};

// ListenerScope<IProjectChangedListener, m_project_interactor, this>;

} // namespace Slic3r::Biz
