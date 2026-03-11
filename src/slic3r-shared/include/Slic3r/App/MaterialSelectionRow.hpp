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
    using FnClicked = std::function<void()>;

    explicit MaterialSelectionRow(
        size_t index,
        const Biz::Preset::PresetItem& data,
        FnClicked on_clicked_extention,
        FnClicked on_cog_clicked,
        size_t& material_index,
        Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;
    void checked_updated_internal() override;
    void hovered_updated_internal() override;
    void update_cog_visibility();

private:
    size_t& m_material_index;
    FnClicked m_on_clicked_extention;
    FnClicked m_on_cog_clicked;
    Biz::Preset::PresetInteractor& m_preset_interactor;
    LayoutButton* m_cog_btn{ nullptr };
};

} // namespace Slic3r::App
