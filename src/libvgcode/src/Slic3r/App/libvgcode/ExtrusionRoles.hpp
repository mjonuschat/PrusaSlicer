///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/libvgcode/Types.hpp"

#include <map>

namespace Slic3r::App::libvgcode {

class ExtrusionRoles
{
public:
    struct Item
    {
        Biz::libpgcode::Times times;
        //
        // first = length in mm
        // second = mass in g
        //  
        std::pair<float, float> used_filament;
    };

    void add(GCodeExtrusionRole role, const std::pair<float, float>& used_filament);
    void update(GCodeExtrusionRole role, const Biz::libpgcode::Times& times);

    size_t roles_count() const { return m_items.size(); }
    Biz::libpgcode::GCodeExtrusionRoles roles() const;
    float time(GCodeExtrusionRole role, Biz::libpgcode::TimeMode mode) const;
    float used_filament_length(GCodeExtrusionRole role) const;
    float used_filament_mass(GCodeExtrusionRole role) const;

    void reset() { m_items.clear(); }

private:
    std::map<GCodeExtrusionRole, Item> m_items;
};

} // namespace Slic3r::App::libvgcode
