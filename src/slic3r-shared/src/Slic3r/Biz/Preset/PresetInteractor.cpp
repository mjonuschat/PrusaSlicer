#include "PresetInteractor.hpp"

namespace Slic3r::Biz::Preset {

bool PresetInteractor::add_change_listener(PresetChangeListener* listener)
{
    const bool already_added =
        std::find(m_change_listeners.begin(), m_change_listeners.end(), listener) !=
        m_change_listeners.end();
    if (!already_added)
        m_change_listeners.push_back(listener);
    return !already_added;
}

bool PresetInteractor::remove_change_listener(PresetChangeListener* listener)
{
    auto it = std::find(m_change_listeners.begin(), m_change_listeners.end(), listener);
    if (it == m_change_listeners.end())
        return false;
    m_change_listeners.erase(it);
    return true;
}


}