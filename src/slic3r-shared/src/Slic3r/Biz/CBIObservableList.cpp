///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/CBIObservableList.hpp"

namespace Slic3r::Biz {

const ConfigBoxInteractor& CBIObservableList::at(size_t index) const
{
    return m_items.at(index);
}

size_t CBIObservableList::size() const
{
    return m_items.size();
}

std::map<const ConfigBoxInteractor*, ConfigBoxInteractor::SetAccessor> CBIObservableList::set_items(
    const std::vector<Domain::ConfigBox*>& config_boxes
)
{
    invoke_listeners<IListObserver<ConfigBoxInteractor>>(
        [&](IListObserver<ConfigBoxInteractor>* l) { l->on_will_be_reset(); }
    );

    m_items.clear();
    m_items.reserve(config_boxes.size());

    std::map<const ConfigBoxInteractor*, ConfigBoxInteractor::SetAccessor> accessor_map;

    for (Domain::ConfigBox* config_box : config_boxes) {
        ConfigBoxInteractor::SetAccessor accessor;
        m_items.emplace_back(accessor, config_box);
        accessor_map[&m_items.back()] = accessor;
    }

    invoke_listeners<IListObserver<ConfigBoxInteractor>>([](IListObserver<ConfigBoxInteractor>* l)
                                                         { l->on_reset(); });

    return accessor_map;
}

} // namespace Slic3r::Biz
