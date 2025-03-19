///|/ Copyright (c) Prusa Research 2018 - 2022 Pavel Mikuš @Godrak, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2014 Petr Ledvina @ledvinap
///|/
///|/ ported from lib/Slic3r/Line.pm:
///|/ Copyright (c) Prusa Research 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <cmath>
#include <limits>
#include <cstdlib>
#include <cstring>

#include "Geometry.hpp"
#include "Line.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

Linef3 transform(const Linef3& line, const Transform3d& t)
{
    typedef Eigen::Matrix<double, 3, 2> LineInMatrixForm;

    LineInMatrixForm world_line;
    ::memcpy((void*)world_line.col(0).data(), (const void*)line.a.data(), 3 * sizeof(double));
    ::memcpy((void*)world_line.col(1).data(), (const void*)line.b.data(), 3 * sizeof(double));

    LineInMatrixForm local_line = t * world_line.colwise().homogeneous();
    return Linef3(Vec3d(local_line(0, 0), local_line(1, 0), local_line(2, 0)), Vec3d(local_line(0, 1), local_line(1, 1), local_line(2, 1)));
}

Vec3d Linef3::intersect_plane(double z) const
{
    auto   v = (this->b - this->a).cast<double>();
    double t = (z - this->a(2)) / v(2);
    return Vec3d(this->a(0) + v(0) * t, this->a(1) + v(1) * t, z);
}

BoundingBox get_extents(const Lines &lines)
{
    BoundingBox bbox;
    for (const Line &line : lines) {
        bbox.merge(line.a);
        bbox.merge(line.b);
    }
    return bbox;
}

} // namespace Slic3r
