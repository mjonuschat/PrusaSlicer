#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Scene {

struct Ray
{
    Vec3d origin;
    Vec3d direction;

    Vec3d point_at(double t) const { return origin + t * direction; }

    /**
     * Tries to find the closest point of @p ray projected onto this ray.
     * @param ray Ray defining set of points which closes
     * @param out_t t parameter to be used to get the actual point on this ray.
     * @return True if the closest projected point was found (and @p out_t was set)
     */
    bool closest_point_from_ray(const Ray& ray, double& out_t) const;
};




}
