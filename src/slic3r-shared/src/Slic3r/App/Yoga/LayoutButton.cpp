#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

Slic3r::App::Yoga::LayoutButton::LayoutButton(const std::string& label, Item* parent)
    : LayoutButton(label, '\0', parent)
{}

Slic3r::App::Yoga::LayoutButton::LayoutButton(const std::string& label, wchar_t icon, Item* parent)
    : AbstractButton(icon, "", parent)
{
    set_orientation(Orientation::Horizontal);

    m_background = new Rectangle(this);
    m_background->set_padding(3);
    m_background->set_fill({65, 65, 65});
    m_background->set_justify_content(YGJustifyCenter);
    m_background->set_align_items(YGAlignCenter);
    m_background->set_hover_effect(true);
    m_background->set_gap(5);
    m_background->set_flex_grow(1);

    m_icon = new Icon(icon, m_background);
    m_icon->set_visible(icon != '\0');

    m_text = new Text(label, m_background);
}

const std::string& Slic3r::App::Yoga::LayoutButton::label() const { return m_text->text(); }

void Slic3r::App::Yoga::LayoutButton::set_label(const std::string& label)
{
    m_text->set_text(label);
}

const ImColor& LayoutButton::background_color() const { return m_background->fill(); }

void LayoutButton::set_background_color(const ImColor& color) { m_background->set_fill(color); }

} // namespace Slic3r::App::Yoga
