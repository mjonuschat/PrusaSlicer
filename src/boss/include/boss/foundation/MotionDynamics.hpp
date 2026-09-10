#pragma once

namespace Slic3r::Boss {

struct MotionDynamics {
    unsigned int acceleration = 0;
    double       minimum_cruise_ratio = 0.0;
    unsigned int jerk = 0;
};

} // namespace Slic3r::Boss
