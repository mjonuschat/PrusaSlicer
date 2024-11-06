#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Scene {
struct Ray;

/**
 * @brief An infinite 3D plane
 */
struct Plane
{

    /**
     * @brief "Normal" vector (may not be normalized)
     *
     * In the plane equation: `ax + by + cz + d = 0` the normal is vector of `(a, b, c)`.
     */
    Vec3f normal;

    /**
     * @brief The `d` coeficient as used in plane equation `ax + by + cz + d = 0.`
     */
    float d;

    /**
     * @brief Creates 3D plane defined by point on the plane and two (non-parallel) vectors in the plane.
     * @param point Point lying on plane
     * @param v0 Vector in one plane direction
     * @param v1 Vector in other plane direction (have to non-parallel to vector @p v0)
     * @return New plane where @p point lays on the plane and @p v0 and @p v1 are vectors
     * also laying on the plane (relatively to @p point).
     */
    static Plane from_point_and_vectors(const Vec3f& point, const Vec3f& v0, const Vec3f& v1);

    /**
     * @brief Create plane defined by three points laying on it.
     * @note @p p0, @p p1 and @p p2 must NOT be collinear (i.e. all three laying on same line).
     * @param p0 First plane point
     * @param p1 Second plane point
     * @param p2 Third plane point
     * @return Plane containing all three points.
     */
    static Plane from_three_points(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2)
    { return from_point_and_vectors(p0, p1 - p0, p2 - p0); }

    /**
     * @brief Tests if @p ray intersects this plane.
     * @param[in] ray Ray definition
     * @param[out] t If `true` returned, @p t will be filled with `t` parameter, which plugged into
     * ray equation (`origin + direction * t`) will get you intersection point.
     * @return `True` if intersection of this plane and @p ray exists, otherwise `false`.
     */
    bool intersects(const Ray& ray, float& t) const;
};
} // namespace Slic3r::App::Scene
