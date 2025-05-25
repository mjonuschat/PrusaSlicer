#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::App::Yoga {

MaterialSettingsButton::MaterialSettingsButton(size_t id, const std::string& tooltip)
    : LayoutButton(tooltip)
{
    set_checkable(true);
    set_max_size({ YGUndefined, 40.f });

    // invalidate vertical padding to use whole button height for separators
    set_content_padding({ 5.f, 0.f });

    size_t index = 0; // it's hidden icon index
    Item* text_index = insert_into_content(std::make_unique<Text>(std::to_string(id)), ++index);
    text_index->set_self_align(YGAlignCenter);

    insert_separator(++index);

    m_color_marker = static_cast<Circle*>(insert_into_content(std::make_unique<Circle>(), ++index));
    m_color_marker->set_height_percent(65);
    m_color_marker->set_self_align(YGAlignCenter);

    ++index;// it's button text index
    expand_label(true);

    insert_separator(++index);

    m_nozzle = static_cast<Text*>(insert_into_content(std::make_unique<Text>(""), ++index));
    m_nozzle->set_self_align(YGAlignCenter);

    set_background_color(ImColor(41, 41, 41));
}

void MaterialSettingsButton::insert_separator(size_t index)
{
    Separator* separator = static_cast<Separator*>(insert_into_content(std::make_unique<Separator>(Orientation::Vertical), index));
    separator->set_fill(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
}

void MaterialSettingsButton::set_color(const ImColor& color)
{
    m_color_marker->set_fill(color);
}

void MaterialSettingsButton::set_nozzle(float nozzle)
{
    m_nozzle->set_text(format("%1%", nozzle));
}

void MaterialSettingsButton::set_material_name(const std::string& name)
{
    set_label(name);
}

} // namespace Slic3r::App::Yoga
