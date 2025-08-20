///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::App {

class MaterialSelectionRow : public Biz::DataObserver<Biz::Preset::PresetItem>, public Yoga::LayoutButton
{
public:
    explicit MaterialSelectionRow(
        size_t index,
        const Biz::Preset::PresetItem& data,
        size_t& material_index,
        Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;

private:
    size_t& m_material_index;
    Biz::Preset::PresetInteractor& m_preset_interactor;
};

} // namespace Slic3r::App
