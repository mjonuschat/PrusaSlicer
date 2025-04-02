///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Enrico Turri
///@enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Filip Sykala @Jony01,
///Lukáš Hejl @hejllukas
///|/ Copyright (c) 2017 Eyal Soha @eyal0
///|/ Copyright (c) Slic3r 2013 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ ported from lib/Slic3r/Geometry.pm:
///|/ Copyright (c) Prusa Research 2017 - 2022 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2013 Jose Luis Perez Diez
///|/ Copyright (c) 2013 Anders Sundman
///|/ Copyright (c) 2013 Jesse Vincent
///|/ Copyright (c) 2012 Mike Sheldrake @mesheldrake
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#pragma once

// Serialization through the Cereal library
#include <cereal/access.hpp>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <cereal/cereal.hpp>
#include <Eigen/Geometry>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <numbers>

#include "Slic3r/Domain/Axis.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Constants.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Polyline.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Math.hpp"

namespace Slic3r::Biz::Algorithms::Geometry {

// Generic result of an orientation predicate.
enum Orientation
{
    ORIENTATION_CCW = 1,
    ORIENTATION_CW = -1,
    ORIENTATION_COLINEAR = 0
};

constexpr double PI = std::numbers::pi;

// Return orientation of the three points (clockwise, counter-clockwise, colinear)
// The predicate is exact for the Domain::coord_t type, using 64bit signed integers for the
// temporaries. which means, the Domain::coord_t types must not have some of the topmost bits
// utilized. As the points are limited to 30 bits + signum, the temporaries u, v, w are limited to
// 61 bits + signum, and d is limited to 63 bits + signum and we are good.
Orientation orient(
    const Domain::Point& a, const Domain::Point& b, const Domain::Point& c
);

// Return orientation of the polygon by checking orientation of the left bottom corner of the polygon
// using exact arithmetics. The input polygon must not contain duplicate points
// (or at least the left bottom corner point must not have duplicates).
bool is_ccw(const Domain::Polygon& poly);

bool ray_ray_intersection(
    const Domain::Vec2d& p1,
    const Domain::Vec2d& v1,
    const Domain::Vec2d& p2,
    const Domain::Vec2d& v2,
    Domain::Vec2d& res
);

bool segment_segment_intersection(
    const Domain::Vec2d& p1,
    const Domain::Vec2d& v1,
    const Domain::Vec2d& p2,
    const Domain::Vec2d& v2,
    Domain::Vec2d& res
);


bool segments_intersect(
    const Domain::Point& ip1,
    const Domain::Point& ip2,
    const Domain::Point& jp1,
    const Domain::Point& jp2
);

Domain::Vec2d foot_pt(const Domain::Line& iline, const Domain::Point& ipt);
Domain::Vec2d foot_pt(
    const Domain::Vec2d& line_pt, const Domain::Vec2d& line_dir, const Domain::Vec2d& pt
);
double ray_point_distance_squared(const Domain::Line& iline, const Domain::Point& ipt);
double ray_point_distance(const Domain::Line& iline, const Domain::Point& ipt);
double ray_point_distance(const Domain::Vec2d& ray_pt, const Domain::Vec2d& ray_dir, const Domain::Vec2d& pt);

// Based on Liang-Barsky function by Daniel White @
// http://www.skytopia.com/project/articles/compsci/clipping.html
template<typename T>
bool liang_barsky_line_clipping_interval(
    // Start and end points of the source line, result will be stored there as well.
    const Domain::Advanced::Vec<T, 2>& x0,
    const Domain::Advanced::Vec<T, 2>& v,
    // Bounding box to clip with.
    const Domain::BoundingBox<T, 2>& bbox,
    std::pair<double, double>& out_interval
)
{
    double t0 = 0.0;
    double t1 = 1.0;
    // Traverse through left, right, bottom, top edges.
    auto clip_side = [&t0, &t1](double p, double q) -> bool {
        if (p == 0) {
            if (q < 0)
                // Domain::Line parallel to the bounding box edge is fully outside of the bounding box.
                return false;
            // else don't clip
        } else {
            double r = q / p;
            if (p < 0) {
                if (r > t1)
                    // Fully clipped.
                    return false;
                if (r > t0)
                    // Partially clipped.
                    t0 = r;
            } else {
                assert(p > 0);
                if (r < t0)
                    // Fully clipped.
                    return false;
                if (r < t1)
                    // Partially clipped.
                    t1 = r;
            }
        }
        return true;
    };

    if (clip_side(-v.x(), -bbox.min.x() + x0.x()) && clip_side(v.x(), bbox.max.x() - x0.x()) &&
        clip_side(-v.y(), -bbox.min.y() + x0.y()) && clip_side(v.y(), bbox.max.y() - x0.y())) {
        out_interval.first = t0;
        out_interval.second = t1;
        return true;
    }
    return false;
}

template<typename T>
bool liang_barsky_line_clipping(
    // Start and end points of the source line, result will be stored there as well.
    Domain::Advanced::Vec<T, 2>& x0,
    Domain::Advanced::Vec<T, 2>& x1,
    // Bounding box to clip with.
    const Domain::BoundingBox<T, 2>& bbox
)
{
    Domain::Advanced::Vec<T, 2> v = x1 - x0;
    std::pair<double, double> interval;
    if (liang_barsky_line_clipping_interval(x0, v, bbox, interval)) {
        // Clipped successfully.
        x1 = x0 + interval.second * v;
        x0 += interval.first * v;
        return true;
    }
    return false;
}

// Based on Liang-Barsky function by Daniel White @
// http://www.skytopia.com/project/articles/compsci/clipping.html
template<typename T>
bool liang_barsky_line_clipping(
    // Start and end points of the source line.
    const Domain::Advanced::Vec<T, 2>& x0src,
    const Domain::Advanced::Vec<T, 2>& x1src,
    // Bounding box to clip with.
    const Domain::BoundingBox<T, 2>& bbox,
    // Start and end points of the clipped line.
    Domain::Advanced::Vec<T, 2>& x0clip,
    Domain::Advanced::Vec<T, 2>& x1clip
)
{
    x0clip = x0src;
    x1clip = x1src;
    return liang_barsky_line_clipping(x0clip, x1clip, bbox);
}

bool directions_parallel(double angle1, double angle2, double max_diff = 0);
bool directions_perpendicular(double angle1, double angle2, double max_diff = 0);
template<class T>
bool contains(const std::vector<T>& vector, const Domain::Point& point);
template<typename T>
T rad2deg(T angle)
{
    return T(180.0) * angle / T(PI);
}
template<typename T>
constexpr T deg2rad(const T angle)
{
    return T(PI) * angle / T(180.0);
}
template<typename T>
T angle_to_0_2PI(T angle)
{
    static const T TWO_PI = T(2) * T(PI);
    while (angle < T(0)) {
        angle += TWO_PI;
    }
    while (TWO_PI < angle) {
        angle -= TWO_PI;
    }

    return angle;
}
template<typename T>
void to_range_pi_pi(T& angle)
{
    if (angle > T(PI) || angle <= -T(PI)) {
        int count = static_cast<int>(std::round(angle / (2 * PI)));
        angle -= static_cast<T>(count * 2 * PI);
        assert(angle <= T(PI) && angle > -T(PI));
    }
}

void simplify_polygons(const Domain::Polygons& polygons, double tolerance, Domain::Polygons* retval);

double linint(double value, double oldmin, double oldmax, double newmin, double newmax);
bool arrange(
    // input
    size_t num_parts,
    const Domain::Vec2d& part_size,
    double gap,
    const Domain::BoundingBox2d* bed_bounding_box,
    // output
    Domain::Vec2ds& positions
);

// Sets the given transform by assembling the given transformations in the following order:
// 1) mirror
// 2) scale
// 3) rotate X
// 4) rotate Y
// 5) rotate Z
// 6) translate
void assemble_transform(
    Domain::Transform3d& transform,
    const Domain::Vec3d& translation = Domain::Vec3d::Zero(),
    const Domain::Vec3d& rotation = Domain::Vec3d::Zero(),
    const Domain::Vec3d& scale = Domain::Vec3d::Ones(),
    const Domain::Vec3d& mirror = Domain::Vec3d::Ones()
);

// Returns the transform obtained by assembling the given transformations in the following order:
// 1) mirror
// 2) scale
// 3) rotate X
// 4) rotate Y
// 5) rotate Z
// 6) translate
Domain::Transform3d assemble_transform(
    const Domain::Vec3d& translation = Domain::Vec3d::Zero(),
    const Domain::Vec3d& rotation = Domain::Vec3d::Zero(),
    const Domain::Vec3d& scale = Domain::Vec3d::Ones(),
    const Domain::Vec3d& mirror = Domain::Vec3d::Ones()
);

// Sets the given transform by multiplying the given transformations in the following order:
// T = translation * rotation * scale * mirror
void assemble_transform(
    Domain::Transform3d& transform,
    const Domain::Transform3d& translation = Domain::Transform3d::Identity(),
    const Domain::Transform3d& rotation = Domain::Transform3d::Identity(),
    const Domain::Transform3d& scale = Domain::Transform3d::Identity(),
    const Domain::Transform3d& mirror = Domain::Transform3d::Identity()
);

// Returns the transform obtained by multiplying the given transformations in the following order:
// T = translation * rotation * scale * mirror
Domain::Transform3d assemble_transform(
    const Domain::Transform3d& translation = Domain::Transform3d::Identity(),
    const Domain::Transform3d& rotation = Domain::Transform3d::Identity(),
    const Domain::Transform3d& scale = Domain::Transform3d::Identity(),
    const Domain::Transform3d& mirror = Domain::Transform3d::Identity()
);

// Sets the given transform by assembling the given translation
void translation_transform(Domain::Transform3d& transform, const Domain::Vec3d& translation);

// Returns the transform obtained by assembling the given translation
Domain::Transform3d translation_transform(const Domain::Vec3d& translation);

// Sets the given transform by assembling the given rotations in the following order:
// 1) rotate X
// 2) rotate Y
// 3) rotate Z
void rotation_transform(Domain::Transform3d& transform, const Domain::Vec3d& rotation);

// Returns the transform obtained by assembling the given rotations in the following order:
// 1) rotate X
// 2) rotate Y
// 3) rotate Z
Domain::Transform3d rotation_transform(const Domain::Vec3d& rotation);

// Sets the given transform by assembling the given scale factors
void scale_transform(Domain::Transform3d& transform, double scale);
void scale_transform(Domain::Transform3d& transform, const Domain::Vec3d& scale);

// Returns the transform obtained by assembling the given scale factors
Domain::Transform3d scale_transform(double scale);
Domain::Transform3d scale_transform(const Domain::Vec3d& scale);

// Returns the euler angles extracted from the given rotation matrix
// Warning -> The matrix should not contain any scale or shear !!!
Domain::Vec3d extract_rotation(const Eigen::Matrix<double, 3, 3, Eigen::DontAlign>& rotation_matrix);

// Returns the euler angles extracted from the given affine transform
// Warning -> The transform should not contain any shear !!!
Domain::Vec3d extract_rotation(const Domain::Transform3d& transform);

class Transformation
{
    Domain::Transform3d m_matrix{Domain::Transform3d::Identity()};

public:
    Transformation() = default;
    explicit Transformation(const Domain::Transform3d& transform) : m_matrix(transform) {}

    Domain::Vec3d get_offset() const { return m_matrix.translation(); }
    double get_offset(Domain::Axis axis) const { return get_offset()[axis]; }

    Domain::Transform3d get_offset_matrix() const;

    void set_offset(const Domain::Vec3d& offset) { m_matrix.translation() = offset; }
    void set_offset(Domain::Axis axis, double offset) { m_matrix.translation()[axis] = offset; }

    Domain::Vec3d get_rotation() const;
    double get_rotation(Domain::Axis axis) const { return get_rotation()[axis]; }

    Domain::Transform3d get_rotation_matrix() const;

    void set_rotation(const Domain::Vec3d& rotation);
    void set_rotation(Domain::Axis axis, double rotation);

    Domain::Vec3d get_scaling_factor() const;
    double get_scaling_factor(Domain::Axis axis) const { return get_scaling_factor()[axis]; }

    Domain::Transform3d get_scaling_factor_matrix() const;

    bool is_scaling_uniform() const
    {
        const Domain::Vec3d scale = get_scaling_factor();
        return std::abs(scale.x() - scale.y()) < 1e-8 && std::abs(scale.x() - scale.z()) < 1e-8;
    }

    void set_scaling_factor(const Domain::Vec3d& scaling_factor);
    void set_scaling_factor(Domain::Axis axis, double scaling_factor);

    Domain::Vec3d get_mirror() const;
    double get_mirror(Domain::Axis axis) const { return get_mirror()[axis]; }

    Domain::Transform3d get_mirror_matrix() const;

    bool is_left_handed() const { return m_matrix.linear().determinant() < 0; }

    void set_mirror(const Domain::Vec3d& mirror);
    void set_mirror(Domain::Axis axis, double mirror);

    bool has_skew() const;

    void reset();
    void reset_offset() { set_offset(Domain::Vec3d::Zero()); }
    void reset_rotation();
    void reset_scaling_factor();
    void reset_mirror() { set_mirror(Domain::Vec3d::Ones()); }
    void reset_skew();

    const Domain::Transform3d& get_matrix() const { return m_matrix; }
    Domain::Transform3d get_matrix_no_offset() const;
    Domain::Transform3d get_matrix_no_scaling_factor() const;

    Domain::Transform3d get_matrix_with_applied_shrinkage_compensation(
        const Domain::Vec3d& shrinkage_compensation
    ) const;

    void set_matrix(const Domain::Transform3d& transform) { m_matrix = transform; }

    Transformation operator*(const Transformation& other) const;

private:
    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(m_matrix);
    }
    explicit Transformation(int) {}
    template<class Archive>
    static void load_and_construct(Archive& ar, cereal::construct<Transformation>& construct)
    {
        // Calling a private constructor with special "int" parameter to indicate that no
        // construction is necessary.
        construct(1);
        ar(construct.ptr()->m_matrix);
    }
};

struct TransformationSVD
{
    Domain::SquareMatrix3d u{Domain::SquareMatrix3d::Identity()};
    Domain::SquareMatrix3d s{Domain::SquareMatrix3d::Identity()};
    Domain::SquareMatrix3d v{Domain::SquareMatrix3d::Identity()};

    bool mirror{false};
    bool scale{false};
    bool anisotropic_scale{false};
    bool rotation{false};
    bool rotation_90_degrees{false};
    bool skew{false};

    explicit TransformationSVD(const Transformation& trafo) : TransformationSVD(trafo.get_matrix())
    {}
    explicit TransformationSVD(const Domain::Transform3d& trafo);

    Eigen::DiagonalMatrix<double, 3, 3> mirror_matrix() const
    {
        return Eigen::DiagonalMatrix<double, 3, 3>(this->mirror ? -1. : 1., 1., 1.);
    }
};

// For parsing a transformation matrix from 3MF / AMF.
extern Domain::Transform3d transform3d_from_string(const std::string& transform_str);

// Rotation when going from the first coordinate system with rotation rot_xyz_from applied
// to a coordinate system with rot_xyz_to applied.
Eigen::Quaterniond rotation_xyz_diff(
    const Domain::Vec3d& rot_xyz_from, const Domain::Vec3d& rot_xyz_to
);
// Rotation by Z to align rot_xyz_from to rot_xyz_to.
// This should only be called if it is known, that the two rotations only differ in rotation around
// the Z axis.
double rotation_diff_z(
    const Domain::Transform3d& trafo_from, const Domain::Transform3d& trafo_to
);

// Is the angle close to a multiple of 90 degrees?
bool is_rotation_ninety_degrees(double a);

// Is the angle close to a multiple of 90 degrees?
bool is_rotation_ninety_degrees(const Domain::Vec3d& rotation);

// Returns true if one transformation may be converted into another transformation by
// rotation around Z and by mirroring in X / Y only. Two objects sharing such transformation
// may share support structures and they share Z height.
bool trafos_differ_in_rotation_by_z_and_mirroring_by_xy_only(
    const Domain::Transform3d& t1, const Domain::Transform3d& t2
);
bool trafos_differ_in_rotation_by_z_and_mirroring_by_xy_only(
    const Transformation& t1, const Transformation& t2
);

template<class Tout = double, class Tin>
std::pair<Tout, Tout> dir_to_spheric(const Domain::Advanced::Vec<Tin, 3>& n, Tout norm = 1.)
{
    Tout z = n.z();
    Tout r = norm;
    Tout polar = std::acos(z / r);
    Tout azimuth = std::atan2(n(1), n(0));
    return {polar, azimuth};
}

template<class T = double>
Domain::Advanced::Vec<T, 3> spheric_to_dir(double polar, double azimuth)
{
    return {
        T(std::cos(azimuth) * std::sin(polar)), T(std::sin(azimuth) * std::sin(polar)),
        T(std::cos(polar))};
}

template<class T = double, class Pair>
Domain::Advanced::Vec<T, 3> spheric_to_dir(const Pair& v)
{
    double plr = std::get<0>(v), azm = std::get<1>(v);
    return spheric_to_dir<T>(plr, azm);
}

/**
 * Checks if a given point is inside a corner of a polygon.
 *
 * The corner of a polygon is defined by three points A, B, C in counterclockwise order.
 *
 * Adapted from CuraEngine LinearAlg2D::isInsideCorner by Tim Kuipers @BagelOrb
 * and @Ghostkeeper.
 *
 * @param a The first point of the corner.
 * @param b The second point of the corner (the common vertex of the two edges forming the corner).
 * @param c The third point of the corner.
 * @param query_point The point to be checked if is inside the corner.
 * @return True if the query point is inside the corner, false otherwise.
 */
bool is_point_inside_polygon_corner(
    const Domain::Point& a,
    const Domain::Point& b,
    const Domain::Point& c,
    const Domain::Point& query_point
);

} // namespace Slic3r::Biz::Algorithms::Geometry
