///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/MaterialSelectionRow.hpp"

#include "Slic3r/Biz/Preset/PresetSelectionCheck.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

MaterialSelectionRow::MaterialSelectionRow(
    size_t index,
    const Biz::Preset::PresetItem& data,
    FnClicked on_clicked_extention,
    FnClicked on_cog_clicked,
    size_t& material_index,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Biz::Preset::PresetItem>(index, data),
    LayoutButton(data.name, Render::Icon::Lock),
    m_material_index(material_index),
    m_on_clicked_extention(on_clicked_extention),
    m_on_cog_clicked(on_cog_clicked),
    m_preset_interactor(preset_interactor)
{
    set_flex_shrink(0);
    set_min_size({30, 30});
    set_expand_label(true);
    set_content_justify_content(YGJustifyFlexStart);
    set_content_direction(YGDirectionRTL);
    set_allow_overlap(true);

    auto switch_matrial = [this]() -> bool
    {
        if (Biz::Preset::PresetSelectionCheck::can_select_material_preset(
                m_preset_interactor,
                m_material_index,
                m_state->id
            ))
        {
            m_preset_interactor.select_material_preset(m_material_index, m_state->id);
            return true;
        }
        return false;
    };

    callbacks().action = [switch_matrial, this]()
    {
        switch_matrial();
        m_on_clicked_extention();
    };

    m_cog_btn = emplace<LayoutButton>(
        1,
        std::string{},
        Render::Icon::Cog,
        Biz::_u8L("Show material settings")
    );
    m_cog_btn->set_self_align(YGAlignCenter);
    m_cog_btn->set_width(22.f);
    m_cog_btn->set_height(22.f);
    m_cog_btn->set_background_color(Platform::Color::ButtonTransparent);
    update_cog_visibility();

    m_cog_btn->callbacks().action = [this, switch_matrial]()
    {
        auto selected_preset = m_preset_interactor.selected_printer_preset();
        if (selected_preset.materials[m_material_index].id != m_state->id) {
            if (!switch_matrial()) {
                return;
            }
        }
        m_on_cog_clicked();
    };

    m_cog_btn->callbacks().hovered_changed = [this](bool) { update_cog_visibility(); };
}

void MaterialSelectionRow::on_data_update()
{
    set_label(m_state->name);
}

void MaterialSelectionRow::checked_updated_internal()
{
    RectangleButton::checked_updated_internal();
    update_cog_visibility();
}

void MaterialSelectionRow::hovered_updated_internal()
{
    RectangleButton::hovered_updated_internal();
    update_cog_visibility();
}

void MaterialSelectionRow::update_cog_visibility()
{
    m_cog_btn->set_visible(hovered() || m_cog_btn->hovered());
}

} // namespace Slic3r::App
