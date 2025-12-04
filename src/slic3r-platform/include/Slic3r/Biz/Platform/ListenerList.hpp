#pragma once

#include <list>
#include <functional>
#include <algorithm>

namespace Slic3r::Biz {

template <class L>
class ListenerList
{
public:
    using ListenerType = L;

    bool add(L* listener)
    {
        if (listener == nullptr
            || std::find(m_listeners.begin(), m_listeners.end(), listener) != m_listeners.end())
        {
            return false; // already exist in listeners
        }
        bool change_next = m_is_next_setted && m_next == m_listeners.end();
        m_listeners.push_back(listener);
        if (change_next) {
            m_next = --m_listeners.end();
        }
        return true;
    }

    bool remove(L* listener)
    {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it == m_listeners.end()) {
            return false; // not a listener
        }
        bool is_invoked = *it == m_invoked;
        bool is_next = m_is_next_setted && it == m_next;
        bool change_next = is_invoked || is_next;
        auto next = m_listeners.erase(it);
        if (change_next) {
            m_is_next_setted = true;
            m_next = next;
        }
        return true;
    }

    void invoke(std::function<void(L*)> func)
    {
        for (auto it = m_listeners.begin(); it != m_listeners.end();) {
            m_invoked = *it;
            func(m_invoked);
            if (m_is_next_setted) {
                m_is_next_setted = false;
                it = m_next;
                m_next = m_listeners.end();
            }
            else {
                ++it;
            }
        }
        m_invoked = nullptr;
    }

private:
    using Listeners = std::list<L*>;
    Listeners m_listeners;

    bool m_is_next_setted = false;
    Listeners::iterator m_next = m_listeners.end();
    L* m_invoked = nullptr;
};
} // namespace Slic3r::Biz
