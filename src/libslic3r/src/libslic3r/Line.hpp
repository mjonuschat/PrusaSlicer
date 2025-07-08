///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Enrico Turri @enricoturri1966
///|/ Copyright (c) 2017 Eyal Soha @eyal0
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ ported from lib/Slic3r/Line.pm:
///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Line_hpp_
#define slic3r_Line_hpp_

#include <type_traits>
#include <cmath>
#include <utility>
#include <vector>
#include <complex>

#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Biz/Algorithms/Line.hpp"
#include "libslic3r.h"
#include "Point.hpp"

namespace Slic3r {

using Line = Slic3r::Domain::Line;
using Lines = Slic3r::Domain::Lines;
using Linef = Slic3r::Domain::Line2d;
using Linesf = Slic3r::Domain::Line2ds;
using Linef3 = Slic3r::Domain::Line3d;

class ThickLine;

typedef std::vector<ThickLine> ThickLines;

class ThickLine : public Line
{
public:
    ThickLine() : a_width(0), b_width(0) {}
    ThickLine(const Point& a, const Point& b) : Line(a, b), a_width(0), b_width(0) {}
    ThickLine(const Point& a, const Point& b, double wa, double wb) : Line(a, b), a_width(wa), b_width(wb) {}

    double a_width, b_width;
};

class CurledLine : public Line
{
public:
    CurledLine() : curled_height(0.0f) {}
    CurledLine(const Point& a, const Point& b) : Line(a, b), curled_height(0.0f) {}
    CurledLine(const Point& a, const Point& b, float curled_height) : Line(a, b), curled_height(curled_height) {}

    float curled_height;
};

using CurledLines = std::vector<CurledLine>;

} // namespace Slic3r

#endif // slic3r_Line_hpp_
