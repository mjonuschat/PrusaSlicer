#pragma once

#include "Slic3r/Domain/Point.hpp"

namespace Slic3r::Biz::Algorithms::Point {

template <Domain::UnscaledVector Vec>
Vec round(const Vec& vector) {
    return {std::round(vector.x()), std::round(vector.y())};
}

/**
 * Checks if the Points contains successive duplicate points.
 *
 * @param points Points to search within.
 * @return true If at least one pair of successive duplicate points is found.
 * @return false Otherwise.
 */
bool has_duplicate_points(const Domain::Points& points);

/**
 * Removes successive duplicate points from the Points.
 *
 * @return true If at least one duplicate point was removed.
 * @return false If no duplicates were found.
 */
bool remove_duplicate_points(Domain::Points& points);

Domain::Points scaled(const std::vector<Domain::Vec2d> &points);

template<typename Derived>
inline Domain::Advanced::Vec<typename Derived::Scalar, 3> to_3d(const Eigen::MatrixBase<Derived> &pt, const typename Derived::Scalar z) {
    static_assert(Derived::IsVectorAtCompileTime && int(Derived::SizeAtCompileTime) == 2, "to_3d(): first parameter is not a 2D vector");
    return { pt.x(), pt.y(), z };
}

} // namespace Slic3r::Biz::Algorithms::Point
