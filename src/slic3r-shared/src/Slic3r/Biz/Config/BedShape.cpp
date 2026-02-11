///|/ Copyright (c) Prusa Research 2018 - 2026 Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv, Filip Sykala @Jony01, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena, Vojtěch Král @vojtechkral
///|/
///|/ ported from lib/Slic3r/GUI/BedShapeDialog.pm:
///|/ Copyright (c) Prusa Research 2016 - 2018 Vojtěch Král @vojtechkral, Vojtěch Bubník @bubnikv
///|/ Copyright (c) 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) 2017 Ahmed Samir Abdelreheem @Samir55
///|/ Copyright (c) Slic3r 2014 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/Config/BedShape.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include <Slic3r/Biz/Algorithms/Tesselate.hpp>
#include "Slic3r/Biz/Algorithms/Geometry/ConvexHull.hpp"

#include <fmt/format.h>

namespace Slic3r::Biz::Config {

static const std::map<BedShape::Parameter, BedShape::ParamAttributes> param_attributes_mapping = {
    {BedShape::Parameter::RectSize,
     {L("Size"), L("Size in X and Y of the rectangular plate."), 0, 1'200, 200}},
    {BedShape::Parameter::RectOrigin,
     {L("Origin"),
      L("Distance of the 0,0 G-code coordinate from the front left corner of the rectangle."),
      -600,
      600,
      0}},
    {BedShape::Parameter::Diameter,
     {L("Diameter"),
      L("Diameter of the print bed. It is assumed that origin (0,0) is located in the center."),
      0,
      600,
      200}},
};

BedShape::BedShape(const std::vector<Domain::Vec2d>& points)
{
    m_bed = Domain::Bed::from(points, 0.f, std::nullopt, std::nullopt, "", "");
}

bool BedShape::is_custom() const
{
    Domain::BedType bed_type = Biz::Algorithms::Bed::detect_bed_type(m_bed);
    return bed_type == Domain::BedType::Convex || bed_type == Domain::BedType::Custom;
}

bool BedShape::is_equal_to(const std::vector<Domain::Vec2d>& points) const
{
    return m_bed.contour() == points;
}

const BedShape::ParamAttributes& BedShape::attributes(Parameter param)
{
    ASSERT(param_attributes_mapping.contains(param));
    return param_attributes_mapping.at(param);
}

static std::string get_param_label(BedShape::Parameter param)
{
    return Biz::_u8(BedShape::attributes(param).name);
}

std::string BedShape::get_type_name(Type type)
{
    switch (type) {
    case Type::Rectangle:
        return Biz::_u8L("Rectangular");
    case Type::Circle:
        return Biz::_u8L("Circular");
    case Type::Custom:
        return Biz::_u8L("Custom");
    }
    // make visual studio happy
    assert(false);
    return {};
}

BedShape::Type BedShape::get_type() const
{
    Domain::BedType bed_type = Biz::Algorithms::Bed::detect_bed_type(m_bed);

    switch (bed_type) {
    case Domain::BedType::Rectangle:
    case Domain::BedType::Invalid:
        return Type::Rectangle;
    case Domain::BedType::Circle:
        return Type::Circle;
    case Domain::BedType::Convex:
    case Domain::BedType::Custom:
        return Type::Custom;
    }
    // make visual studio happy
    assert(false);
    return Type::Rectangle;
}

std::vector<Domain::Vec2d> BedShape::triangles() const
{
    Domain::ExPolygon polygon(Slic3r::Biz::Algorithms::Polygon::scaled(m_bed.contour()));
    return Slic3r::Biz::Algorithms::Tesselate::triangulate_expolygon_2d(polygon, false);
}

const std::vector<Domain::Vec2d>& BedShape::contour() const
{
    return m_bed.contour();
}

using namespace Slic3r::Biz::Algorithms;

Domain::Vec2d BedShape::get_size() const
{
    return m_bed.contour_aabb_extent();
}

Domain::Vec2d BedShape::get_origin() const
{
    const Domain::Vec2ds& contour = m_bed.contour();
    ASSERT(contour.size() > 3);

    // Calculate various metrics of the input polygon.
    Domain::Polygon polygon      = Polygon::scaled(contour);
    Domain::Polygon convex_hull  = Geometry::convex_hull(polygon);
    Domain::BoundingBox2crd bbox = Polygon::get_extents(convex_hull);
    return Scaling::unscaled<double>(bbox.min);
}

double BedShape::get_diameter() const
{
    ASSERT(Bed::detect_bed_type(m_bed) == Domain::BedType::Circle);

    Geometry::Circled circle = Bed::as_circular_bed(m_bed);
    return 2. * (circle.radius);
    return 0.0;
}

std::string BedShape::get_full_name_with_params() const
{
    Domain::BedType bed_type = Bed::detect_bed_type(m_bed);

    std::string out = Biz::_u8L("Shape") + ": " + get_type_name(this->get_type());
    switch (bed_type) {
    case Domain::BedType::Circle: {
        const double diameter = get_diameter();
        out += "\n" + get_param_label(Parameter::Diameter) + fmt::format(": [{:.10g}]", diameter);
    } break;
    default: {
        Domain::Vec2d size   = get_size();
        Domain::Vec2d bb_min = get_origin();
        if (bb_min != Domain::Vec2d::Zero())
            bb_min *= -1.;

        out += "\n"
            + get_param_label(Parameter::RectSize)
            + fmt::format(": [{:.10g}, {:.10g}]\n", size.x(), size.y());
        out += get_param_label(Parameter::RectOrigin)
            + fmt::format(": [{:.10g}, {:.10g}]\n", bb_min.x(), bb_min.y());
    } break;
    }
    return out;
}

} // namespace Slic3r::Biz::Config
