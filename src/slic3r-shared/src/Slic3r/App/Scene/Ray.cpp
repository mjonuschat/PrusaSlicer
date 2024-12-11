#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Scene {

bool Ray::closest_point_from_ray(const Ray& ray, double& out_t) const
{
    auto a = ray.direction.dot(ray.direction);
    auto b = direction.dot(ray.direction);
    if (b == 1 || b == -1)
        return false;
    auto c = direction.dot(direction);
    auto w0 = ray.origin - origin;
    auto d = ray.direction.dot(w0);
    auto e = direction.dot(w0);
    out_t = (a * e - b * d) / (a * c - b * b);
    return true;
}

}
