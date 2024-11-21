#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Scene {

struct Ray
{
    Vec3d origin;
    Vec3d direction;

    Vec3d point_at(double t) const { return origin + t * direction; }
};




}
