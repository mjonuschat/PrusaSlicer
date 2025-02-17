///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <cstdint>
#include <cstddef>

namespace Slic3r::Biz::libvgcode {

class OptionTemplate
{
public:
    OptionTemplate() = default;
    ~OptionTemplate() = default;
    OptionTemplate(const OptionTemplate& other) = delete;
    OptionTemplate(OptionTemplate&& other) = delete;
    OptionTemplate& operator = (const OptionTemplate& other) = delete;
    OptionTemplate& operator = (OptionTemplate&& other) = delete;

    //
    // Initialize gpu buffers.
    //
    void init(uint8_t resolution);
    //
    // Release gpu buffers.
    //
    void render(size_t count);
};

} // namespace Slic3r::Biz::libvgcode
