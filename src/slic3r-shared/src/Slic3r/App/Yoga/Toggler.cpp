#include "Slic3r/App/Yoga/Toggler.hpp"

#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Yoga {

Toggler::Toggler()
{
    set_align_items(YGAlignCenter);
    set_disabled_fill(ImColor(95, 95, 95));

    set_width(20);
    set_height(14);
    set_padding(3);

    m_knob = this->emplace_back<Circle>();
    m_knob->set_height_percent(100);
    m_knob->set_disabled_fill(ImColor(20, 20, 20));
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
