///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/libvgcode/Types.hpp"

#include <float.h>

namespace Slic3r::App::libvgcode {

class ToolMarker
{
public:
    ToolMarker() = default;
    ~ToolMarker() = default;
    ToolMarker(const ToolMarker& other) = delete;
    ToolMarker(ToolMarker&& other) = delete;
    ToolMarker& operator = (const ToolMarker& other) = delete;
    ToolMarker& operator = (ToolMarker&& other) = delete;

    //
    // Initialize gpu buffers.
    //
    void init(uint16_t resolution, float tip_radius, float tip_height, float stem_radius, float stem_height);
    //
    // Render the marker
    //
    void render();

    float offset_z() const { return m_offset_z; }
    void set_offset_z(float offset_z) { m_offset_z = std::max(offset_z, 0.0f); }

    const ColorRGB& color() const { return m_color; }
    void set_color(const ColorRGB& color) { m_color = color; }

    float alpha() const { return m_alpha; }
    void set_alpha(float alpha) { m_alpha = std::clamp(alpha, 0.25f, 0.75f); }

    const BoundingBoxf3& bounding_box() const { return m_bounding_box; }

private:
    float m_offset_z{ 0.5f };
    ColorRGB m_color{ ColorRGB::WHITE() };
    float m_alpha{ 0.5f };
    BoundingBoxf3 m_bounding_box;
};

} // namespace Slic3r::App::libvgcode
