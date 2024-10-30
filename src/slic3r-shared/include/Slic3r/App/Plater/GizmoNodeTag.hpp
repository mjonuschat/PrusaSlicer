#pragma once

#include <cstdint>

namespace Slic3r::App::Plater {

enum class AxisType : uint8_t {
    None = 0,
    XAxis = 1,
    YAxis = 2,
    ZAxis = 3
};

struct GizmoNodeTag {
    const AxisType primary_axis;
    const AxisType secondary_axis{AxisType::None};
};

}
