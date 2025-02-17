///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/libvgcode/Types.hpp"

namespace Slic3r::Biz::libvgcode {

class CogMarker
{
public:
    CogMarker() = default;
    ~CogMarker() = default;
    CogMarker(const CogMarker& other) = delete;
    CogMarker(CogMarker&& other) = delete;
    CogMarker& operator = (const CogMarker& other) = delete;
    CogMarker& operator = (CogMarker&& other) = delete;

    //
    // Initialize gpu buffers
    //
    void init(uint8_t resolution, float radius);
    //
    // Render the marker
    //
    void render();
    //
    // Update values used to calculate the center of gravity
    //
    void update(const Vec3f& position, float mass);
    //
    // Reset values used to calculate the center of gravity
    //
    void reset();
    //
    // Return the calculated center of gravity position
    //
    Vec3f position() const;
    //
    // Return the total mass.
    //
    float total_mass() const { return m_total_mass; }

private:
    //
    // Values used to calculate the center of gravity
    //
    float m_total_mass{ 0.0f };
    Vec3f m_total_position{ Vec3f::Zero() };
};

} // namespace Slic3r::Biz::libvgcode
