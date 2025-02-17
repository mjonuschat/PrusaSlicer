///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Range.hpp"

namespace Slic3r::Biz::libvgcode {

class Layers
{
public:
    void update(const libpgcode::MoveVertex& vertex, uint32_t vertex_id);
    void update_as_sla(float z, float time);

    void reset();
    
    bool empty() const { return m_items.empty(); }
    size_t count() const { return m_items.size(); }

    std::vector<float> times(libpgcode::TimeMode mode) const;
    std::vector<float> zs() const;
    
    float layer_time(libpgcode::TimeMode mode, size_t layer_id) const {
        return (mode < libpgcode::TimeMode::COUNT && layer_id < m_items.size()) ?
            m_items[layer_id].times[size_t(mode)] : 0.0f;
    }
    float layer_z(size_t layer_id) const {
        return (layer_id < m_items.size()) ? m_items[layer_id].z : 0.0f;
    }
    size_t layer_id_at(float z) const;
    
    const Interval& view_range() const { return m_view_range.get(); }
    void set_view_range(const Interval& range) { set_view_range(range[0], range[1]); }
    void set_view_range(Interval::value_type min, Interval::value_type max) { m_view_range.set(min, max); }
    
    bool layer_contains_colorprint_options(size_t layer_id) const {
        return (layer_id < m_items.size()) ? m_items[layer_id].contains_colorprint_options : false;
    }

private:
    struct Item
    {
        float z{ 0.0f };
        Range range;
        libpgcode::Times times{};
        bool contains_colorprint_options{ false };
    };
    
    std::vector<Item> m_items;
    Range m_view_range;
};

} // namespace Slic3r::Biz::libvgcode
