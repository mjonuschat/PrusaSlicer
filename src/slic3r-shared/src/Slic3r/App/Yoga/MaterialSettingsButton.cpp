#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::App::Yoga {

MaterialSettingsButton::MaterialSettingsButton(
    size_t index,
    const Biz::Preset::PresetItemObservableList& state,
    std::weak_ptr<ButtonGroup> button_group
) :
    RectangleButton(format(_u8L("Material %1% TT"), index + 1)),
    Biz::DataObserver<Biz::Preset::PresetItemObservableList>(index, state),
    m_button_group(button_group)
{
    set_checkable(true);
    set_max_size({YGUndefined, 40.f});
    set_flex_shrink(0);

    // invalidate vertical padding to use whole button height for separators
    set_content_padding({5.f, 0.f});

    Item* text_index = emplace_back<Text>(std::to_string(index + 1));
    text_index->set_self_align(YGAlignCenter);

    emplace_back<Separator>(Orientation::Vertical)
        ->set_fill(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

    m_color_marker = emplace_back<Circle>();
    m_color_marker->set_height_percent(65);
    m_color_marker->set_self_align(YGAlignCenter);

    m_material_name = emplace_back<Text>("");
    m_material_name->set_flex_grow(1.f);
    m_material_name->set_self_align(YGAlignCenter);

    emplace_back<Separator>(Orientation::Vertical)
        ->set_fill(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

    m_nozzle = emplace_back<Text>("");
    m_nozzle->set_self_align(YGAlignCenter);

    set_background_color(ImColor(41, 41, 41));

    on_data_update();

    m_button_group.lock()->insert_button(this);
}

MaterialSettingsButton::~MaterialSettingsButton()
{
    if (!m_button_group.expired()) {
        m_button_group.lock()->remove_button(this);
    }
}

void MaterialSettingsButton::set_color(const ImColor& color)
{
    m_color_marker->set_fill(color);
}

void MaterialSettingsButton::set_nozzle(float nozzle)
{
    m_nozzle->set_text(format("%1%", nozzle));
}

void MaterialSettingsButton::on_data_update()
{
    on_list_selection_changed(m_state->selected_index());
}

void MaterialSettingsButton::on_list_selection_changed(Domain::SelectionId new_selection)
{
    const_cast<Biz::Preset::PresetItemObservableList*>(m_state)
        ->add_listener<Biz::IListSelectionChangedListener>(this);

    const Biz::Preset::PresetItem& preset_item = m_state->items().at(new_selection);

    const std::string prefix{preset_item.runtime_only ? _u8L("(From 3mf) ") : ""};
    set_material_name(prefix + preset_item.name);
    set_color(ImColor(250, 104, 48));
    set_nozzle(0.4);
}

void MaterialSettingsButton::set_material_name(const std::string& name)
{
    m_material_name->set_text(name);
}

} // namespace Slic3r::App::Yoga
