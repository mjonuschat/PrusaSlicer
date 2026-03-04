///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/OverridableCBIObservableList.hpp"

namespace Slic3r::Biz {

const OverridableConfigBoxInteractor& OverridableCBIObservableList::at(size_t index) const
{
    return m_items.at(index);
}

size_t OverridableCBIObservableList::size() const
{
    return m_items.size();
}

std::map<const OverridableConfigBoxInteractor*, OverridableConfigBoxInteractor::SetAccessor>
OverridableCBIObservableList::set_items(const std::vector<Domain::ConfigBox*>& config_boxes)
{
    invoke_listeners<IListObserver<OverridableConfigBoxInteractor>>(
        [&](IListObserver<OverridableConfigBoxInteractor>* l) { l->on_will_be_reset(); }
    );

    m_items.clear();
    m_items.reserve(config_boxes.size());

    std::map<const OverridableConfigBoxInteractor*, OverridableConfigBoxInteractor::SetAccessor>
        accessor_map;

    for (Domain::ConfigBox* config_box : config_boxes) {
        OverridableConfigBoxInteractor::SetAccessor accessor;
        m_items.emplace_back(accessor, config_box);
        accessor_map[&m_items.back()] = accessor;
    }

    invoke_listeners<IListObserver<OverridableConfigBoxInteractor>>(
        [](IListObserver<OverridableConfigBoxInteractor>* l) { l->on_reset(); }
    );

    return accessor_map;
}

} // namespace Slic3r::Biz
