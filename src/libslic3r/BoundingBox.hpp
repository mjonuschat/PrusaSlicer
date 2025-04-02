///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Filip Sykala @Jony01, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2014 - 2015 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_BoundingBox_hpp_
#define slic3r_BoundingBox_hpp_

#include <assert.h>
#include <algorithm>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstddef>

#include "libslic3r.h"
#include "Slic3r/Exception.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"

namespace Slic3r {
class BoundingBox;

// TEMPORARY, until BoundingBox is replace by algorithms bounding box.
namespace Impl {
    template<class T, class O = T>
    using IteratorOnly = std::enable_if_t<
        !std::is_same_v<typename std::iterator_traits<T>::value_type, void>, O
    >;
}

template <typename PointType, typename APointsType = std::vector<PointType>>
class [[deprecated("Use Domain::BoundingBox")]] BoundingBoxBase : public Domain::BoundingBox<typename PointType::Scalar, PointType::RowsAtCompileTime>
{
    using ParentType = Domain::BoundingBox<typename PointType::Scalar, PointType::RowsAtCompileTime>;
public:
    using PointsType = APointsType;

    [[deprecated("Use Domain::BoundingBox instead")]]
    BoundingBoxBase() = default;
    [[deprecated("Use Domain::BoundingBox instead")]]
    BoundingBoxBase(const PointType &pmin, const PointType &pmax) : ParentType{pmin, pmax} {}
    [[deprecated("Use Biz::Algorithms::BoundingBox::construct instead")]]
    BoundingBoxBase(const PointType& p1, const PointType& p2, const PointType& p3)
    {
        namespace bb = Biz::Algorithms::BoundingBox;
        this->min = p1;
        this->max = p1;
        this->defined = false;

        *this = bb::merge(*this, p2);
        *this = bb::merge(*this, p3);
    }

    template<class It, class = Impl::IteratorOnly<It>>
    [[deprecated("Use Biz::Algorithms::BoundingBox::construct instead")]]
    BoundingBoxBase(It from, It to)
    {
        namespace bb = Biz::Algorithms::BoundingBox;
        const auto box{bb::construct(from, to)};
        this->min = box.min;
        this->max = box.max;
        this->defined = box.defined;
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::construct instead")]]
    BoundingBoxBase(const PointsType &points)
        : BoundingBoxBase(points.begin(), points.end())
    {}

    [[deprecated("Use Domain::BoundingBox box = {} instead")]]
    void reset() { this->defined = false; this->min = PointType::Zero(); this->max = PointType::Zero(); }

    [[deprecated("Use Biz::Algorithms::BoundingBox::merge instead")]]
    void merge(const PointType &point);

    [[deprecated("Use Biz::Algorithms::BoundingBox::merge(Biz::Algorithms::BoundingBox::construct()) instead")]]
    void merge(const PointsType &points);

    [[deprecated("Use Biz::Algorithms::BoundingBox::merge instead")]]
    void merge(const BoundingBoxBase<PointType, PointsType> &bb);

    [[deprecated("Did you mean to use inflate()? If not, do the scaling directly on min and max.")]]
    void scale(double factor);

    [[deprecated("Use Biz::Algorithms::BoundingBox::sizes instead")]]
    PointType size() const;

    [[deprecated("Calculate the radius directly 0.5 * (bb.max - bb.min).norm()")]]
    double radius() const;

    [[deprecated("Use Biz::Algorithms::BoundingBox::translated")]]
    void translate(double x, double y) {
        using Biz::Algorithms::BoundingBox::translated;
        const ParentType box{this->min, this->max, this->defined};
        const auto result{translated(box, PointType{x, y})};
        this->min = result.min;
        this->max = result.max;
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::translated")]]
    void translate(const PointType &v) {
        using Biz::Algorithms::BoundingBox::translated;
        const ParentType box{this->min, this->max, this->defined};
        const auto result{translated(box, v)};
        this->min = result.min;
        this->max = result.max;
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::inflated")]]
    void offset(double delta);

    [[deprecated("Use Biz::Algorithms::BoundingBox::inflated")]]
    BoundingBoxBase<PointType, PointsType> inflated(double delta) const throw() { BoundingBoxBase<PointType, PointsType> out(*this); out.offset(delta); return out; }

    [[deprecated("Use Biz::Algorithms::BoundingBox::center")]]
    PointType center() const;

    [[deprecated("Use Biz::Algorithms::BoundingBox::contains")]]
    bool contains(const PointType &point) const {
        using Biz::Algorithms::BoundingBox::contains;
        const ParentType box{this->min, this->max, this->defined};
        return contains(box, point);
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::contains")]]
    bool contains(const BoundingBoxBase<PointType, PointsType> &other) const {
        using Biz::Algorithms::BoundingBox::contains;
        const ParentType box{this->min, this->max, this->defined};
        const ParentType other_box{other.min, other.max, other.defined};
        return contains(box, other_box);
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::overlap")]]
    bool overlap(const BoundingBoxBase<PointType, PointsType> &other) const {
        using Biz::Algorithms::BoundingBox::overlap;
        const ParentType box{this->min, this->max, this->defined};
        const ParentType other_box{other.min, other.max, other.defined};
        return overlap(box, other_box);
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::operator== for coord_t or aprox_equals for floating values")]]
    bool operator==(const BoundingBoxBase<PointType, PointsType> &rhs) const noexcept {
        const ParentType box{this->min, this->max, this->defined};
        const ParentType rhs_box{rhs.min, rhs.max, rhs.defined};
        if constexpr (std::is_same_v<typename PointType::Scalar, Domain::coord_t>) {
            return box == rhs_box;
        } else {
            using Biz::Algorithms::BoundingBox::approx_equals;
            return approx_equals(box, rhs_box);
        }
    }

    [[deprecated("Use Biz::Algorithms::BoundingBox::operator== for coord_t or aprox_equals for floating values")]]
    bool operator!=(const BoundingBoxBase<PointType, PointsType> &rhs) const noexcept { return ! (*this == rhs); }
};

template <class PointType>
using BoundingBox3Base = BoundingBoxBase<PointType, std::vector<PointType>>;

// Will prevent warnings caused by non existing definition of template in hpp
extern template void     BoundingBoxBase<Point, Points>::scale(double factor);
extern template void     BoundingBoxBase<Vec2d>::scale(double factor);
extern template void     BoundingBoxBase<Vec3d>::scale(double factor);
extern template void     BoundingBoxBase<Point, Points>::offset(double delta);
extern template void     BoundingBoxBase<Vec2d>::offset(double delta);
extern template void     BoundingBoxBase<Point, Points>::merge(const Point &point);
extern template void     BoundingBoxBase<Vec2f>::merge(const Vec2f &point);
extern template void     BoundingBoxBase<Vec2d>::merge(const Vec2d &point);
extern template void     BoundingBoxBase<Point, Points>::merge(const Points &points);
extern template void     BoundingBoxBase<Vec2d>::merge(const Pointfs &points);
extern template void     BoundingBoxBase<Point, Points>::merge(const BoundingBoxBase<Point, Points> &bb);
extern template void     BoundingBoxBase<Vec2f>::merge(const BoundingBoxBase<Vec2f> &bb);
extern template void     BoundingBoxBase<Vec2d>::merge(const BoundingBoxBase<Vec2d> &bb);

extern template void     BoundingBoxBase<Vec3f>::merge(const Vec3f &point);
extern template void     BoundingBoxBase<Vec3d>::merge(const Vec3d &point);
extern template void     BoundingBoxBase<Vec3d>::merge(const Pointf3s &points);
extern template void     BoundingBoxBase<Vec3d>::merge(const BoundingBoxBase<Vec3d> &bb);

extern template Point    BoundingBoxBase<Point, Points>::size() const;
extern template Vec2f    BoundingBoxBase<Vec2f>::size() const;
extern template Vec2d    BoundingBoxBase<Vec2d>::size() const;

extern template Vec3f    BoundingBoxBase<Vec3f>::size() const;
extern template Vec3d    BoundingBoxBase<Vec3d>::size() const;

extern template double   BoundingBoxBase<Point, Points>::radius() const;
extern template double   BoundingBoxBase<Vec2d>::radius() const;
extern template double   BoundingBoxBase<Vec3d>::radius() const;

extern template Point    BoundingBoxBase<Point, Points>::center() const;
extern template Vec2f    BoundingBoxBase<Vec2f>::center() const;
extern template Vec2d    BoundingBoxBase<Vec2d>::center() const;

extern template void     BoundingBoxBase<Vec3d>::offset(double delta);
extern template Vec3f    BoundingBoxBase<Vec3f>::center() const;
extern template Vec3d    BoundingBoxBase<Vec3d>::center() const;

class [[deprecated("Use Domain::BoundingBox")]] BoundingBox : public BoundingBoxBase<Point, Points>
{
public:
    [[deprecated("Use Domain::BoundingBox")]]
    BoundingBox(const BoundingBoxBase<Point, Points>& box): BoundingBoxBase{box} {}

    [[deprecated("Use Domain::BoundingBox")]]
    BoundingBox(const BoundingBoxBase<Vec2crd> &bb): BoundingBox(bb.min, bb.max) {}

    void polygon(Polygon* polygon) const;
    Polygon polygon() const;

    [[deprecated("BB rotation has unclear meaning, rotate min, max directly")]]
    BoundingBox rotated(double angle) const;

    [[deprecated("BB rotation has unclear meaning, rotate min, max directly")]]
    BoundingBox rotated(double angle, const Point &center) const;

    [[deprecated("BB rotation has unclear meaning, rotate min, max directly")]]
    void rotate(double angle) { (*this) = this->rotated(angle); }

    [[deprecated("BB rotation has unclear meaning, rotate min, max directly")]]
    void rotate(double angle, const Point &center) { (*this) = this->rotated(angle, center); }

    // Align the min corner to a grid of cell_size x cell_size cells,
    // to encompass the original bounding box.
    [[deprecated("Do not use this, implement it wherever it is needed")]]
    void align_to_grid(const coord_t cell_size);

    using BoundingBoxBase::BoundingBoxBase;

    [[deprecated("Use Biz::Algorithms::BoundingBox::inflated")]]
    BoundingBox inflated(double delta) const noexcept { BoundingBox out(*this); out.offset(delta); return out; }

    [[deprecated("Did you mean to use inflate()? If not, do the scaling directly on min and max.")]]
    BoundingBox scaled(double factor) const;
};

using  BoundingBoxes = std::vector<BoundingBox>;

class [[deprecated("Use Domain::BoundingBox")]] BoundingBox3  : public BoundingBoxBase<Vec3crd>
{
public:
    using BoundingBoxBase::BoundingBoxBase;
};

class [[deprecated("Use Domain::BoundingBox")]] BoundingBoxf : public BoundingBoxBase<Vec2d>
{
public:
    using BoundingBoxBase::BoundingBoxBase;

    BoundingBoxf(const BoundingBoxBase<Vec2d>& box): BoundingBoxBase{box} {}
};

class [[deprecated("Use Domain::BoundingBox")]] BoundingBoxf3 : public BoundingBoxBase<Vec3d>
{
public:
    using BoundingBoxBase::BoundingBoxBase;

    BoundingBoxf3 transformed(const Transform3d& matrix) const;
};

template<typename PointType, typename PointsType>
[[deprecated("Use Biz::Algorithms::BoundingBox::empty")]]
inline bool empty(const BoundingBoxBase<PointType, PointsType> &bb)
{
    using Biz::Algorithms::BoundingBox::empty;

    const Domain::BoundingBox<typename PointType::Scalar, PointType::RowsAtCompileTime> box{
        bb.min, bb.max, bb.defined
    };

    return empty(box);
}

[[deprecated("Use Biz::Algorithms::BoundingBox::scaled instead")]]
inline BoundingBox scaled(const BoundingBoxf &bb) {
    using Biz::Algorithms::BoundingBox::scaled;

    const Domain::BoundingBox2d box{
        bb.min, bb.max, bb.defined
    };
    const auto result{scaled(box)};

    return {result.min, result.max};
}

template<class T = coord_t, class Tin>
[[deprecated("Use Biz::Algorithms::BoundingBox::scaled instead")]]
BoundingBoxBase<LegacyVec<2, T>> scaled(const BoundingBoxBase<LegacyVec<2, Tin>> &bb)
{
    using Biz::Algorithms::BoundingBox::scaled;

    const Domain::BoundingBox<Tin, 2> box{
        bb.min, bb.max, bb.defined
    };
    const auto result{scaled<T>(box)};

    return {result.min, result.max};
}

template<class T = double, class Tin>
[[deprecated("Use Biz::Algorithms::BoundingBox::unscaled instead")]]
BoundingBoxBase<LegacyVec<2, T>> unscaled(const BoundingBoxBase<LegacyVec<2, Tin>> &bb) {
    using Biz::Algorithms::BoundingBox::unscaled;

    const Domain::BoundingBox<Tin, 2> box{
        bb.min, bb.max, bb.defined
    };
    const auto result{unscaled<T>(box)};

    return {result.min, result.max};
}

template<class T = double>
[[deprecated("Use Biz::Algorithms::BoundingBox::unscaled instead")]]
BoundingBoxBase<LegacyVec<2, T>> unscaled(const BoundingBox &bb) {
    using Biz::Algorithms::BoundingBox::unscaled;

    const Domain::BoundingBox2crd box{
        bb.min, bb.max, bb.defined
    };
    const auto result{unscaled<T>(box)};

    return {result.min, result.max};
}

template<class Tout, class Tin>
[[deprecated("Use Biz::Algorithms::BoundingBox::cast instead")]]
auto cast(const BoundingBoxBase<Tin> &b)
{
    using Biz::Algorithms::BoundingBox::cast;

    const Domain::BoundingBox<typename Tin::Scalar, Tin::RowsAtCompileTime> box{
        b.min, b.max, b.defined
    };
    const auto result{cast<Tout>(box)};
    return BoundingBoxBase<LegacyVec<Tin::RowsAtCompileTime, Tout>>{
        result.min,
        result.max
    };
}

// Distance of a point to a bounding box. Zero inside and on the boundary, positive outside.
[[deprecated("Use Biz::Algorithms::BoundingBox::distance")]]
inline double bbox_point_distance(const BoundingBox &bbox, const Point &pt)
{
    using Biz::Algorithms::BoundingBox::distance;

    const Domain::BoundingBox2crd box{
        bbox.min, bbox.max, bbox.defined
    };

    return distance(box, pt);
}

[[deprecated("Use Biz::Algorithms::BoundingBox::distance_squared")]]
inline double bbox_point_distance_squared(const BoundingBox &bbox, const Point &pt)
{
    using Biz::Algorithms::BoundingBox::distance_squared;

    const Domain::BoundingBox2crd box{
        bbox.min, bbox.max, bbox.defined
    };

    return distance_squared(box, pt);
}

// Minimum distance between two Bounding boxes.
// Returns zero when Bounding boxes overlap.
[[deprecated("Use Biz::Algorithms::BoundingBox::distance")]]
inline double bbox_bbox_distance(const BoundingBox &first_bbox, const BoundingBox &second_bbox) {
    using Biz::Algorithms::BoundingBox::distance;

    const Domain::BoundingBox2crd a{
        first_bbox.min, first_bbox.max, first_bbox.defined
    };
    const Domain::BoundingBox2crd b{
        second_bbox.min, second_bbox.max, second_bbox.defined
    };

    return distance(a, b);
}

template<class T>
[[deprecated("Use Biz::Algorithms::BoundingBox::to_2d")]]
BoundingBoxBase<LegacyVec<2, T>> to_2d(const BoundingBoxBase<LegacyVec<3, T>> &bb)
{

    using Biz::Algorithms::BoundingBox::to_2d;

    const Domain::BoundingBox<T, 3> box{
        bb.min, bb.max, bb.defined
    };
    const auto result{to_2d(box)};

    return {result.min, result.max};
}

} // namespace Slic3r

// Serialization through the Cereal library
namespace cereal {
	template<class Archive> void serialize(Archive& archive, Slic3r::BoundingBox   &bb) { archive(bb.min, bb.max, bb.defined); }
	template<class Archive> void serialize(Archive& archive, Slic3r::BoundingBox3  &bb) { archive(bb.min, bb.max, bb.defined); }
	template<class Archive> void serialize(Archive& archive, Slic3r::BoundingBoxf  &bb) { archive(bb.min, bb.max, bb.defined); }
	template<class Archive> void serialize(Archive& archive, Slic3r::BoundingBoxf3 &bb) { archive(bb.min, bb.max, bb.defined); }
}

#endif
