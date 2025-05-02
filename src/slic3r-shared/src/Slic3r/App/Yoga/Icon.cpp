#include "Slic3r/App/Yoga/Icon.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Yoga {

Icon::Icon(wchar_t icon, Item *parent) : Item(parent), m_icon(icon) {
    set_aspect_ratio(1.);
}

void Icon::render(Vec2f pos, Vec2f size)
{
    if (!m_parent) {
        style_node();
        resize(size);
    }

    Imgui::icon_image(m_icon, to_im(size), !enabled());

    render_internal(pos, size);
}

wchar_t Icon::icon() const { return m_icon; }

void Icon::set_icon(wchar_t icon) { m_icon = icon; }

} // namespace Slic3r::App::Yoga
