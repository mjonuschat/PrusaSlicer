#include "Slic3r/App/Yoga/RadioExtruder.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga {

RadioExtruder::RadioExtruder(size_t number, ImColor fill) :
    AbstractButton(),
    m_number_text(number),
    m_fill(fill)
{
    set_aspect_ratio(1);

    m_marker = emplace_back<Circle>();
    m_marker->set_fill(IM_COL32_BLACK_TRANS);

    m_knob = m_marker->emplace_back<Circle>();
    m_knob->set_justify_content(YGJustifyCenter);
    m_knob->set_align_items(YGAlignCenter);
    m_knob->set_fill(fill);
    m_knob->set_border_color(GImGui->Style.Colors[ImGuiCol_WindowBg]);
    m_knob->set_border_width(m_border_width);

    m_text = m_knob->emplace_back<Text>(std::to_string(number));
    m_text->set_font_type(Render::ImguiFontType::Bold);
    m_text->set_margin({0, -1, 0, 0});
    m_text->set_text_color(GImGui->Style.Colors[ImGuiCol_WindowBg]);
}

const ImColor& RadioExtruder::fill() const
{
    return m_fill;
}

void RadioExtruder::set_fill(const ImColor& fill)
{
    m_knob->set_fill(fill);
}

size_t RadioExtruder::number() const
{
    return m_number_text;
}

void RadioExtruder::set_number(size_t num)
{
    m_number_text = num;
}

float RadioExtruder::border_width() const
{
    return m_border_width;
}

void RadioExtruder::set_border_width(float border)
{
    m_border_width = border;
    m_marker->set_padding(m_border_width + 1);
    m_knob->set_border_width(checked() ? m_border_width : 0);
}

void RadioExtruder::checked_updated_internal()
{
    AbstractButton::checked_updated_internal();
    if (checked()) {
        m_marker->set_fill(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
    } else {
        m_marker->set_fill(IM_COL32_BLACK_TRANS);
    }

    m_knob->set_border_width(checked() ? m_border_width : 0);
}

} // namespace Slic3r::App::Yoga
