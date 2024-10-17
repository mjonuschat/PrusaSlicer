#pragma once

#include <cstddef>

namespace Slic3r::App::Render {


/**
 * Screen resolution info including conversion between logical (scaled) and physical (real) pixels.
 */
class ScreenInfo {
public:
    /**
     * Creates screen info
     * @param width Width in physical pixels (without applied scale)
     * @param height Height in physical pixels (without applied scale)
     * @param scale The UI scale which applied to logical pixel size gains physical one.
     */
    ScreenInfo(size_t width, size_t height, float scale)
        : m_width(width), m_height(height), m_scale(scale)
    {}

    size_t physical_width() const { return m_width; }
    size_t physical_height() const { return m_height; }
    float logical_width() const { return m_width / m_scale; }
    float logical_height() const { return m_height / m_scale; }

    float logical_to_physical(float logical_coord) const { return logical_coord * m_scale; }
    float physical_to_logical(float physical_coord) const { return physical_coord / m_scale; }

    float physical_to_imgui_x(float physical_x) const { return physical_to_logical(physical_x); }
    float physical_to_imgui_y(float physical_y) const { return physical_to_logical(physical_height() - physical_y - 1); }

    float mouse_to_screen(float mouse_coord) { return logical_to_physical(mouse_coord); }

    bool operator==(const ScreenInfo& rhs) const
    {
        return m_width == rhs.m_width && m_height == rhs.m_height && m_scale == rhs.m_scale;
    }

    bool operator!=(const ScreenInfo& rhs) const
    {
        return !(*this == rhs);
    }

private:
    size_t m_width;
    size_t m_height;
    float m_scale;
};

}
