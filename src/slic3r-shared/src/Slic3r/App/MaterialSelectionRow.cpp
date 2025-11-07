///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/MaterialSelectionRow.hpp"

#include "Slic3r/Biz/Preset/PresetSelectionCheck.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

MaterialSelectionRow::MaterialSelectionRow(
    size_t index,
    const Biz::Preset::PresetItem& data,
    size_t& material_index,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Biz::Preset::PresetItem>(index, data),
    LayoutButton(data.name, Render::Icon::Lock),
    m_material_index(material_index),
    m_preset_interactor(preset_interactor)
{
    set_flex_shrink(0);
    set_min_size({30, 30});
    set_expand_label(true);
    set_content_justify_content(YGJustifyFlexStart);
    set_content_direction(YGDirectionRTL);
    callbacks().action = [this]()
    {
        if (Biz::Preset::PresetSelectionCheck::can_select_material_preset(
                m_preset_interactor,
                m_material_index,
                m_state->id
            ))
        {
            m_preset_interactor.select_material_preset(m_material_index, m_state->id);
        }
    };
}

void MaterialSelectionRow::on_data_update()
{
    set_label(m_state->name);
}

} // namespace Slic3r::App
