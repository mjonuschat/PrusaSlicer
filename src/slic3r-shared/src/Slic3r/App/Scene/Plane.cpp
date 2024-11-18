#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/Ray.hpp"

namespace Slic3r::App::Scene {

Plane Plane::from_point_and_vectors(const Vec3d& point, const Vec3d& v0, const Vec3d& v1)
{
    const Vec3d normal = v0.cross(v1);
    const double d = -normal.dot(point);
    return {normal, d};
}

bool Plane::intersects(const Ray& ray, double& t) const
{
    const double denom = ray.direction.dot(normal);
    if (std::fabs(denom) <= std::numeric_limits<double>::epsilon())
        // the ray and plane are parallel => the only case there is no intersection
        return false;

    t = - (ray.origin.dot(normal) + d) / denom;
    return true;
}

}
