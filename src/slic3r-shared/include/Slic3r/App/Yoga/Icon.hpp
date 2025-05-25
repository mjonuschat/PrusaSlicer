///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Render {
class Texture;
}

namespace Slic3r::App::Yoga {

/**
 * @brief The Icon class can be used to display Image assets (PNG or SVG) files
 * inside the Yoga Layout tree.
 *
 * The only way how to set icon currently is to use Render::Icon, we would like
 * to preserve that, because we can easily track which assets we are actually
 * using and where in the code they are.
 * Also the assets path will be correctly resolved, because we want to use
 * combination of built in assets and those loaded from web etc.
 *
 * Icons can be created with either explicit size or a dynamic resize.
 * The size in question is a size of the GPU Texture, texture is always
 * loaded scaled with a preserver aspect ratio according to the dynamic or explicit size.
 *
 * For rendering fill mode of the texture can be changed as well. By default fill mode
 * is set to PreservedAspect which will rescale Texture with Texture aspect ratio to this
 * Item size.
 * E.g. Loaded Icon has aspect ratio 2x1 and is SVG, Icon instance has min_size and max_size set to
 * 50x50, that means the Texture will be loaded as 25x50 and will cover upper 50% of the whole
 * item size.
 */
class Icon : public Item
{
public:

    enum class PreferredSize : int {
        S = 32,
        M = 64,
        L = 128,
    };

    enum class FillMode {
        PreservedAspect,
        Stretch
    };

    /// \note auto_resize = false
    Icon(Render::Icon icon, PreferredSize explicit_max_size);
    /// \note auto_resize = false
    Icon(Render::Icon icon, int explicit_max_size);
    /// \note auto_resize = true
    Icon(Render::Icon icon);

    void render(Vec2f pos, Vec2f size) override;

    Render::Icon icon() const;
    void set_icon(Render::Icon icon);

    const Vec2f& source_size() const;
    void set_source_size(const Vec2f& source_size);

    FillMode fill_mode() const;
    void set_fill_mode(FillMode fill_mode);

private:
    void update_draw_sizes();
    void update_texture();

private:
    // Configured
    bool m_auto_resize = true;
    int m_max_texture_size{0};
    std::shared_ptr<Render::Texture> m_texture;
    Render::Icon m_icon;
    Vec2f m_source_size{0, 0};
    FillMode m_fill_mode = FillMode::PreservedAspect;

    // Computed
    Vec2f m_cached_size;
    ImVec2 m_draw_size;
};

} // namespace Slic3r::App::Yoga
