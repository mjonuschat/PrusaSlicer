///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "MultiPoint.hpp"

#include <cmath>
#include <limits>
#include <queue>
#include <cstdlib>

#include "BoundingBox.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

BoundingBox MultiPoint::bounding_box() const
{
    return BoundingBox(this->points);
}

void MultiPoint3::translate(double x, double y)
{
    for (Vec3crd &p : points) {
        p(0) += coord_t(x);
        p(1) += coord_t(y);
    }
}

void MultiPoint3::translate(const Point& vector)
{
    this->translate(vector(0), vector(1));
}

BoundingBox3 MultiPoint3::bounding_box() const
{
    return BoundingBox3(points);
}

bool MultiPoint3::remove_duplicate_points()
{
    size_t j = 0;
    for (size_t i = 1; i < points.size(); ++i) {
        if (points[j] == points[i]) {
            // Just increase index i.
        } else {
            ++ j;
            if (j < i)
                points[j] = points[i];
        }
    }

    if (++j < points.size())
    {
        points.erase(points.begin() + j, points.end());
        return true;
    }

    return false;
}

BoundingBox get_extents(const MultiPoint &mp)
{ 
    return BoundingBox(mp.points);
}

BoundingBox get_extents_rotated(const Points &points, double angle)
{ 
    BoundingBox bbox;
    if (! points.empty()) {
        double s = sin(angle);
        double c = cos(angle);
        Points::const_iterator it = points.begin();
        double cur_x = (double)(*it)(0);
        double cur_y = (double)(*it)(1);
        bbox.min(0) = bbox.max(0) = (coord_t)round(c * cur_x - s * cur_y);
        bbox.min(1) = bbox.max(1) = (coord_t)round(c * cur_y + s * cur_x);
        for (++it; it != points.end(); ++it) {
            double cur_x = (double)(*it)(0);
            double cur_y = (double)(*it)(1);
            coord_t x = (coord_t)round(c * cur_x - s * cur_y);
            coord_t y = (coord_t)round(c * cur_y + s * cur_x);
            bbox.min(0) = std::min(x, bbox.min(0));
            bbox.min(1) = std::min(y, bbox.min(1));
            bbox.max(0) = std::max(x, bbox.max(0));
            bbox.max(1) = std::max(y, bbox.max(1));
        }
        bbox.defined = true;
    }
    return bbox;
}

BoundingBox get_extents_rotated(const MultiPoint &mp, double angle)
{
    return get_extents_rotated(mp.points, angle);
}

}
