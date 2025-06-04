#include "Slic3r/App/Yoga/Toggler.hpp"

#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Yoga {

static constexpr float round_up_to_even(float value)
{
    int int_value = value;
    return int_value % 2 == 0 ? int_value : ++int_value;
}

Toggler::Toggler()
{
    set_align_items(YGAlignCenter);

    float text_height = round_up_to_even(0.9f * std::max(ImGui::GetTextLineHeight(), 12.f));

    Vec2f size = { round_up_to_even(1.5f * text_height), text_height };

    set_width(size.x());
    set_height(size.y());
    set_padding(0.2f * size.y());

    m_knob = this->emplace_back<Circle>();
    m_knob->set_height_percent(100);
}

static ImVec4 bg_color(bool checked, bool hovered)
{
    return hovered ? ImVec4(0.675f, 0.675f, 0.675f, 1.0f)
        : checked
        ? ImVec4(0.85f, 0.85f, 0.85f, 1.0f)
        : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
}

static ImVec4 knob_color(bool checked)
{
    return checked 
        ? ImVec4(0.31f, 0.51f, 0.97f, 1.0f)
        : ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
}

void Toggler::process_events(Vec2f pos, Vec2f size)
{
    Rectangle::process_events(pos, size);

    AbstractButton* parent = static_cast<AbstractButton*>(this->parent());
    const bool checked = parent->checked();
    set_fill(bg_color(checked, parent->hovered()));
    m_knob->set_fill(knob_color(checked));
}

void Toggler::set_checked(bool checked)
{
    set_justify_content(checked ? YGJustifyFlexEnd : YGJustifyFlexStart);
}

} // namespace Slic3r::App::Yoga
