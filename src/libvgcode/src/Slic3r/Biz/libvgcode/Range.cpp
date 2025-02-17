///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "Range.hpp"

#include <algorithm>

namespace Slic3r::Biz::libvgcode {

void Range::set(Interval::value_type min, Interval::value_type max)
{
    if (max < min)
        std::swap(min, max);
    m_range[0] = min;
    m_range[1] = max;
}

void Range::clamp(Range& other)
{
    other.m_range[0] = std::clamp(other.m_range[0], m_range[0], m_range[1]);
    other.m_range[1] = std::clamp(other.m_range[1], m_range[0], m_range[1]);
}

} // namespace Slic3r::Biz::libvgcode
