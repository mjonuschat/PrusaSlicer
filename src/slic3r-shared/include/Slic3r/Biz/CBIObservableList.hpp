///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

namespace Slic3r::Biz {

class CBIObservableList : public IObservableList<ConfigBoxInteractor>
{
public:

    virtual const ConfigBoxInteractor& at(size_t index) const;

    virtual size_t size() const;

    std::map<const ConfigBoxInteractor*, ConfigBoxInteractor::SetAccessor> set_items(
        const std::vector<Domain::ConfigBox*>& config_boxes
    );

private:
    std::vector<ConfigBoxInteractor> m_items;
};

} // namespace Slic3r::Biz
