///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/ObservableListTransformer.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/PageEntryButton.hpp"

namespace Slic3r::App {

class CategoryPageTransformer : public Biz::ObservableListTransformer<Domain::ConfigItem, PageEntry> {
public:

    CategoryPageTransformer();
};

}
