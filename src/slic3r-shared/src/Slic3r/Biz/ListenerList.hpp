#pragma once

#include <deque>
#include <functional>
#include <algorithm>

namespace Slic3r::Biz {

template <class L>
class ListenerList {
public:
    bool add(L* listener)
    {
        if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end()) {
            m_listeners.push_back(listener);
            return true;
        }
        return false;
    }

    bool remove(L* listener)
    {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it != m_listeners.end()) {
            m_listeners.erase(it);
            return true;
        }
        return false;
    }

    void invoke(std::function<void(L*)> func)
    {
        std::for_each(m_listeners.begin(), m_listeners.end(), func);
    }
private:
    std::deque<L*> m_listeners;

};
}

