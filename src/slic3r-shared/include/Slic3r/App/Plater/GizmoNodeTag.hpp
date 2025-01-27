#pragma once

#include <cstdint>
#include <libslic3r/Point.hpp>
#include <libslic3r/Color.hpp>

namespace Slic3r::App::Plater {

enum class AxisType : uint8_t
{
    None = 0,
    XAxis = 1,
    YAxis = 2,
    ZAxis = 3
};

Vec3d axis_type_dir(AxisType at);

struct GizmoNodeTag
{
    const AxisType primary_axis;
    const AxisType secondary_axis{AxisType::None};

    explicit GizmoNodeTag(AxisType primary_axis, AxisType secondary_axis = AxisType::None)
        : primary_axis(primary_axis), secondary_axis(secondary_axis)
    {}

    Vec3d primary_axis_dir() const { return axis_type_dir(primary_axis); }
    Vec3d secondary_axis_dir() const { return axis_type_dir(secondary_axis); }
};

inline Vec3d axis_type_dir(AxisType at)
{
    Vec3d ret = Vec3d::Zero();
    if (at != AxisType::None)
        ret[int(at) - 1] = 1;
    return ret;
}

inline ColorRGBA axis_color(AxisType axis)
{
    switch (axis)
    {
    case AxisType::XAxis: { return ColorRGBA::X(); }
    case AxisType::YAxis: { return ColorRGBA::Y(); }
    case AxisType::ZAxis: { return ColorRGBA::Z(); }
    default:              { return ColorRGBA::BLACK(); }
    }
}

inline std::string axis_string(AxisType axis)
{
    switch (axis)
    {
    case AxisType::XAxis: { return "X axis"; }
    case AxisType::YAxis: { return "Y axis"; }
    case AxisType::ZAxis: { return "Z axis"; }
    default:              { return "?"; }
    }
}

}
