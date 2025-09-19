///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/OverrideItem.hpp"

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class OverrideCategoryRow : public Biz::DataObserver<Biz::OverrideItem>, public Yoga::Item
{
public:
    explicit OverrideCategoryRow(
        size_t index,
        const Biz::OverrideItem& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
};

} // namespace Slic3r::App
