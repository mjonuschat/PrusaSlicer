#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/Ray.hpp"

namespace Slic3r::App::Scene {

Plane Plane::from_point_and_vectors(const Vec3f& point, const Vec3f& v0, const Vec3f& v1)
{
    const Vec3f normal = v0.cross(v1);
    const float d = -normal.dot(point);
    return {normal, d};
}

bool Plane::intersects(const Ray& ray, float& t) const
{
    const float denom = ray.direction.dot(normal);
    if (std::fabs(denom) <= std::numeric_limits<float>::epsilon())
        // the ray and plane are parallel => the only case there is no intersection
        return false;

    t = - (ray.origin.dot(normal) + d) / denom;
    return true;
}

}
