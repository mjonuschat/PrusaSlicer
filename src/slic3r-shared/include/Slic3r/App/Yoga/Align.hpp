///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"

namespace Slic3r::App::Yoga {

enum class AlignH {
    Left,
    Center,
    Right
};

enum class AlignV {
    Top,
    Center,
    Bottom
};

struct Align {
    AlignH  horizontal  { AlignH::Left };
    AlignV  vertical    { AlignV::Center };

    YGAlign get_yoga_v_align()
    {
        return vertical == AlignV::Top    ? YGAlignFlexStart :
               vertical == AlignV::Bottom ? YGAlignFlexEnd : YGAlignCenter;
    }

    YGAlign get_yoga_h_align()
    {
        return horizontal == AlignH::Left   ? YGAlignFlexStart :
               horizontal == AlignH::Right ? YGAlignFlexEnd : YGAlignCenter;
    }
};

}

