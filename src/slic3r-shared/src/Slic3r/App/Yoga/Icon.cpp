///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/Icon.hpp"

#include <Slic3r/App/Render/Texture.hpp>
#include <Slic3r/App/Render/ImguiRender.hpp>

namespace Slic3r::App::Yoga {

Icon::Icon(Render::Icon icon, PreferredSize explicit_max_size)
    : Icon(icon, static_cast<int>(explicit_max_size))
{}

Icon::Icon(Render::Icon icon, int explicit_max_size)
    : m_auto_resize(false), m_max_texture_size(explicit_max_size)
{
    set_icon(icon);
}

Icon::Icon(Render::Icon icon) : m_auto_resize(true), m_icon(icon) {}

void Icon::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    if (m_icon != Render::Icon::None &&
        (!Domain::fuzzy_compare(m_cached_size.x(), size.x()) ||
         !Domain::fuzzy_compare(m_cached_size.y(), size.y()))) {
        m_cached_size = size;
        if (m_auto_resize) {
            int new_max_size = std::max(size.x(), size.y());
            if (m_max_texture_size != new_max_size) {
                m_max_texture_size = new_max_size;
                update_texture();
            }
        }
        update_draw_sizes();
    }

    ImGui::SetCursorScreenPos(to_im(pos));
    if (m_texture) {
        constexpr ImVec2 uv0{0, 0};
        constexpr ImVec2 uv1{1, 1};

        ImGui::Image((ImTextureID) (intptr_t) m_texture.get(), m_draw_size, uv0, uv1, m_tint);
    }

    render_item_end(pos, size);
}

Render::Icon Icon::icon() const { return m_icon; }

void Icon::set_icon(Render::Icon icon)
{
    if (m_icon == icon) {
        return;
    }

    m_icon = icon;
    update_texture();
    update_draw_sizes();
}

const Vec2f& Icon::source_size() const { return m_source_size; }

void Icon::set_source_size(const Vec2f& source_size) { m_source_size = source_size; }

Icon::FillMode Icon::fill_mode() const { return m_fill_mode; }

void Icon::set_fill_mode(FillMode fill_mode)
{
    if (m_fill_mode != fill_mode) {
        m_fill_mode = fill_mode;
        if (m_texture) {
            update_draw_sizes();
        }
    }
}

ImColor Icon::tint() const
{
    return {m_tint};
}

void Icon::set_tint(const ImColor &tint)
{
    m_tint = tint;
}

void Icon::update_draw_sizes()
{
    if (!m_texture) {
        return;
    }

    ImVec2 component_size(YGNodeLayoutGetWidth(m_node), YGNodeLayoutGetHeight(m_node));

    if (m_fill_mode == FillMode::Stretch) {
        m_draw_size = component_size;
    } else if (m_fill_mode == FillMode::PreservedAspect) {
        Render::Size size(m_texture->width(), m_texture->height());
        size.scale(Render::Size(component_size.x, component_size.y));
        m_draw_size.x = size.width;
        m_draw_size.y = size.height;
    }
}

void Icon::update_texture()
{
    if (m_icon == Render::Icon::None || m_max_texture_size == 0) {
        m_texture = nullptr;
    } else {
        m_texture = m_imgui_render->icon_texture(m_icon, m_max_texture_size);
    }
}

} // namespace Slic3r::App::Yoga
