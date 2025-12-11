#include "Slic3r/App/Yoga/Toggler.hpp"

#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

Toggler::Toggler()
{
    set_align_items(YGAlignCenter);
    set_disabled_fill(ImColor(95, 95, 95));

    set_width(20);
    set_height(14);
    set_padding(3);

    m_knob = emplace_back<Circle>();
    m_knob->set_height_percent(100);
    m_knob->set_disabled_fill(ImColor(20, 20, 20));

    m_inner_oval = emplace_back<Oval>();
    m_inner_oval->set_width_percent(100);
    m_inner_oval->set_height_percent(100);
    m_inner_oval->set_disabled_fill(ImColor(20, 20, 20));
    m_inner_oval->set_visible(false);
}

void Toggler::update_contents()
{
    YGJustify justify;
    if (m_third_state) {
        justify = YGJustify::YGJustifyCenter;
    } else if (m_checked) {
        justify = YGJustify::YGJustifyFlexEnd;
    } else {
        justify = YGJustify::YGJustifyFlexStart;
    }

    set_justify_content(justify);

    m_knob->set_visible(!m_third_state);
    m_inner_oval->set_visible(m_third_state);
}

ImColor Toggler::bg_color(bool hovered) const
{
    if (hovered) {
        return ImColor(0.675f, 0.675f, 0.675f, 1.0f);
    }

    return m_checked ? ImColor(0.85f, 0.85f, 0.85f, 1.0f) : ImColor(0.5f, 0.5f, 0.5f, 1.0f);
}

ImColor Toggler::knob_color() const
{
    return m_third_state || m_checked ? ImColor(0.31f, 0.51f, 0.97f, 1.0f) :
                                        ImColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void Toggler::style_node()
{
    AbstractButton* parent = static_cast<AbstractButton*>(this->parent());
    set_fill(bg_color(parent->hovered()));

    ImColor inner_color = knob_color();
    m_knob->set_fill(inner_color);
    m_inner_oval->set_fill(inner_color);
}

bool Toggler::third_state() const
{
    return m_third_state;
}

void Toggler::set_third_state(bool third_state)
{
    m_third_state = third_state;
    update_contents();
}

void Toggler::set_checked(bool checked)
{
    m_third_state = false;
    m_checked     = checked;
    update_contents();
}

} // namespace Slic3r::App::Yoga
