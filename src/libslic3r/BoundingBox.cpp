///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2014 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2014 Petr Ledvina @ledvinap
///|/
///|/ ported from lib/Slic3r/Geometry/BoundingBox.pm:
///|/ Copyright (c) Slic3r 2013 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "BoundingBox.hpp"

#include <algorithm>

#include "libslic3r/Point.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

namespace bb = Biz::Algorithms::BoundingBox;

template BoundingBoxBase<Point, Points>::BoundingBoxBase(const Points &points);
template BoundingBoxBase<Vec2d>::BoundingBoxBase(const std::vector<Vec2d> &points);


void BoundingBox::polygon(Polygon* polygon) const
{
    polygon->points = { 
        this->min,
        { this->max.x(), this->min.y() },
        this->max,
        { this->min.x(), this->max.y() }
    };
}

Polygon BoundingBox::polygon() const
{
    Polygon p;
    this->polygon(&p);
    return p;
}

BoundingBox BoundingBox::rotated(double angle) const
{
    using Domain::rotated;

    BoundingBox out;
    out.merge(rotated(this->min, angle));
    out.merge(rotated(this->max, angle));
    out.merge(Point(this->min.x(), this->max.y()).rotated(angle));
    out.merge(Point(this->max.x(), this->min.y()).rotated(angle));
    return out;
}

BoundingBox BoundingBox::rotated(double angle, const Point &center) const
{
    using Domain::rotated;

    BoundingBox out;
    out.merge(rotated(this->min, angle, center));
    out.merge(rotated(this->max, angle, center));
    out.merge(Point(this->min.x(), this->max.y()).rotated(angle, center));
    out.merge(Point(this->max.x(), this->min.y()).rotated(angle, center));
    return out;
}

BoundingBox BoundingBox::scaled(double factor) const
{
    BoundingBox out(*this);
    out.scale(factor);
    return out;
}

template <class PointType, typename APointsType> void
BoundingBoxBase<PointType, APointsType>::scale(double factor)
{
    this->min *= factor;
    this->max *= factor;
}
template void BoundingBoxBase<Point, Points>::scale(double factor);
template void BoundingBoxBase<Vec2d>::scale(double factor);
template void BoundingBoxBase<Vec3d>::scale(double factor);

template <class PointType, typename APointsType> void
BoundingBoxBase<PointType, APointsType>::merge(const PointType &point)
{
    const ParentType box{this->min, this->max, this->defined};
    const auto result{bb::merge(box, point)};
    this->min = result.min;
    this->max = result.max;
    this->defined = result.defined;
}
template void BoundingBoxBase<Point, Points>::merge(const Point &point);
template void BoundingBoxBase<Vec2f>::merge(const Vec2f &point);
template void BoundingBoxBase<Vec2d>::merge(const Vec2d &point);
template void BoundingBoxBase<Vec3f>::merge(const Vec3f &point);
template void BoundingBoxBase<Vec3d>::merge(const Vec3d &point);

template <class PointType, typename APointsType> void
BoundingBoxBase<PointType, APointsType>::merge(const PointsType &points)
{
    this->merge(BoundingBoxBase(points));
}
template void BoundingBoxBase<Point, Points>::merge(const Points &points);
template void BoundingBoxBase<Vec2d>::merge(const Pointfs &points);
template void BoundingBoxBase<Vec3d>::merge(const Pointf3s &points);

template <class PointType, typename APointsType> void
BoundingBoxBase<PointType, APointsType>::merge(const BoundingBoxBase<PointType, PointsType> &bb)
{
    const ParentType box_a{this->min, this->max, this->defined};
    const ParentType box_b{bb.min, bb.max, bb.defined};
    const auto result{bb::merge(box_a, box_b)};
    this->min = result.min;
    this->max = result.max;
    this->defined = result.defined;
}
template void BoundingBoxBase<Point, Points>::merge(const BoundingBoxBase<Point, Points> &bb);
template void BoundingBoxBase<Vec2f>::merge(const BoundingBoxBase<Vec2f> &bb);
template void BoundingBoxBase<Vec2d>::merge(const BoundingBoxBase<Vec2d> &bb);
template void BoundingBoxBase<Vec3d>::merge(const BoundingBoxBase<Vec3d> &bb);

template <class PointType, typename APointsType> PointType
BoundingBoxBase<PointType, APointsType>::size() const
{
    using Biz::Algorithms::BoundingBox::sizes;
    const ParentType box{this->min, this->max, this->defined};
    return sizes(box);
}
template Point BoundingBoxBase<Point, Points>::size() const;
template Vec2f BoundingBoxBase<Vec2f>::size() const;
template Vec2d BoundingBoxBase<Vec2d>::size() const;
template Vec3f BoundingBoxBase<Vec3f>::size() const;
template Vec3d BoundingBoxBase<Vec3d>::size() const;

template <class PointType, typename APointsType> double BoundingBoxBase<PointType, APointsType>::radius() const
{
    assert(this->defined);
    return 0.5 * (this->max - this->min).template cast<double>().norm();
}
template double BoundingBoxBase<Point, Points>::radius() const;
template double BoundingBoxBase<Vec2d>::radius() const;
template double BoundingBoxBase<Vec3d>::radius() const;

template <class PointType, typename APointsType> void
BoundingBoxBase<PointType, APointsType>::offset(double delta)
{
    const ParentType box{this->min, this->max, this->defined};
    using Biz::Algorithms::BoundingBox::inflated;
    const auto result{inflated(box, static_cast<typename PointType::Scalar>(delta))};
    this->min = result.min;
    this->max = result.max;
}
template void BoundingBoxBase<Point, Points>::offset(double delta);
template void BoundingBoxBase<Vec2d>::offset(double delta);
template void BoundingBoxBase<Vec3d>::offset(double delta);

template <class PointType, typename APointsType> PointType
BoundingBoxBase<PointType, APointsType>::center() const
{
    return (this->min + this->max) / 2;
}
template Point BoundingBoxBase<Point, Points>::center() const;
template Vec2f BoundingBoxBase<Vec2f>::center() const;
template Vec2d BoundingBoxBase<Vec2d>::center() const;
template Vec3f BoundingBoxBase<Vec3f>::center() const;
template Vec3d BoundingBoxBase<Vec3d>::center() const;

void BoundingBox::align_to_grid(const coord_t cell_size)
{
    if (this->defined) {
        min.x() = Slic3r::align_to_grid(min.x(), cell_size);
        min.y() = Slic3r::align_to_grid(min.y(), cell_size);
    }
}

BoundingBoxf3 BoundingBoxf3::transformed(const Transform3d& matrix) const
{
    typedef Eigen::Matrix<double, 3, 8, Eigen::DontAlign> Vertices;

    Vertices src_vertices;
    src_vertices(0, 0) = min.x(); src_vertices(1, 0) = min.y(); src_vertices(2, 0) = min.z();
    src_vertices(0, 1) = max.x(); src_vertices(1, 1) = min.y(); src_vertices(2, 1) = min.z();
    src_vertices(0, 2) = max.x(); src_vertices(1, 2) = max.y(); src_vertices(2, 2) = min.z();
    src_vertices(0, 3) = min.x(); src_vertices(1, 3) = max.y(); src_vertices(2, 3) = min.z();
    src_vertices(0, 4) = min.x(); src_vertices(1, 4) = min.y(); src_vertices(2, 4) = max.z();
    src_vertices(0, 5) = max.x(); src_vertices(1, 5) = min.y(); src_vertices(2, 5) = max.z();
    src_vertices(0, 6) = max.x(); src_vertices(1, 6) = max.y(); src_vertices(2, 6) = max.z();
    src_vertices(0, 7) = min.x(); src_vertices(1, 7) = max.y(); src_vertices(2, 7) = max.z();

    Vertices dst_vertices = matrix * src_vertices.colwise().homogeneous();

    Vec3d v_min(dst_vertices(0, 0), dst_vertices(1, 0), dst_vertices(2, 0));
    Vec3d v_max = v_min;

    for (int i = 1; i < 8; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            v_min(j) = std::min(v_min(j), dst_vertices(j, i));
            v_max(j) = std::max(v_max(j), dst_vertices(j, i));
        }
    }

    return BoundingBoxf3(v_min, v_max);
}

}
