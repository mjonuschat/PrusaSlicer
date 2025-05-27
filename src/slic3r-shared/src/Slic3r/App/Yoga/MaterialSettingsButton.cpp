#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::App::Yoga {

MaterialSettingsButton::MaterialSettingsButton(size_t id, const std::string& tooltip)
    : RectangleButton(tooltip)
{
    set_checkable(true);
    set_max_size({ YGUndefined, 40.f });

    // invalidate vertical padding to use whole button height for separators
    set_content_padding({ 5.f, 0.f });

    Item* text_index = emplace_back<Text>(std::to_string(id));
    text_index->set_self_align(YGAlignCenter);

    emplace_back<Separator>(Orientation::Vertical)->set_fill(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

    m_color_marker = emplace_back<Circle>();
    m_color_marker->set_height_percent(65);
    m_color_marker->set_self_align(YGAlignCenter);

    m_material_name = emplace_back<Text>("");
    m_material_name->set_flex_grow(1.f);
    m_material_name->set_self_align(YGAlignCenter);

    emplace_back<Separator>(Orientation::Vertical)->set_fill(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

    m_nozzle = emplace_back<Text>("");
    m_nozzle->set_self_align(YGAlignCenter);

    set_background_color(ImColor(41, 41, 41));
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
    m_material_name->set_text(name);
}

} // namespace Slic3r::App::Yoga
