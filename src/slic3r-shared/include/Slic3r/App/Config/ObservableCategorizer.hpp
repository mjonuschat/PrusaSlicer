///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Biz/ConfigItemContext.hpp>
#include "Slic3r/Biz/ObservableListSortFilter.hpp"

namespace Slic3r::App {

class ObservableCategorizer :
    public Biz::ObservableListSortFilter<Biz::ConfigItemContext, Domain::ConfigItemDef::Category>
{
public:
    ObservableCategorizer();
};

} // namespace Slic3r::App
