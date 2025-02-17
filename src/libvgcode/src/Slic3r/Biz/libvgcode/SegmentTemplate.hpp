///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <cstddef>

namespace Slic3r::Biz::libvgcode {

class SegmentTemplate
{
public:
    SegmentTemplate() = default;
    ~SegmentTemplate() = default;
    SegmentTemplate(const SegmentTemplate& other) = delete;
    SegmentTemplate(SegmentTemplate&& other) = delete;
    SegmentTemplate& operator = (const SegmentTemplate& other) = delete;
    SegmentTemplate& operator = (SegmentTemplate&& other) = delete;

    //
    // Initialize gpu buffers.
    //
    void init();

    void render(size_t count);
};

} // namespace Slic3r::Biz::libvgcode
