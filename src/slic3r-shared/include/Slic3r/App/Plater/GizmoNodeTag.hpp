#pragma once

#include <cstdint>
#include <libslic3r/Point.hpp>

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



}
