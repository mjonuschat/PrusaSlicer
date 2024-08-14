#pragma once

#include <vector>
#include "Slic3r/Domain/Workbench.hpp"

namespace Slic3r::Biz::Preset {

class PresetChangeListener;

class PresetInteractor final {
public:
    explicit PresetInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    PresetInteractor(PresetInteractor&&) = default;

    bool add_change_listener(PresetChangeListener* listener);
    bool remove_change_listener(PresetChangeListener* listener);

private:
    Domain::Workbench& m_workbench;
    std::vector<PresetChangeListener*> m_change_listeners;
};
}
