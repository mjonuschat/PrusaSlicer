///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/Biz/libvgcode/Types.hpp"

namespace Slic3r::Biz::libvgcode {

static uint8_t lerp(uint8_t f1, uint8_t f2, float t)
{
    float one_minus_t = 1.0f - t;
    return uint8_t(one_minus_t * float(f1) + t * float(f2));
}

std::string light_reference_system_to_string(LightReferenceSystem sys)
{
    switch (sys)
    {
    case LightReferenceSystem::Eye:   { return "Eye"; }
    case LightReferenceSystem::World: { return "World"; }
    default:                          { return "Unknown"; }
    }
}

} // namespace Slic3r::Biz::libvgcode
