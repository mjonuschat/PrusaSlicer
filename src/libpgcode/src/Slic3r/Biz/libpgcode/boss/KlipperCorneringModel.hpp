#pragma once

namespace Slic3r::Boss {

struct KlipperCorneringModel {
    static constexpr float kSqrt2Minus1 = 0.41421356237f;

    static float junction_deviation_from_scv(float square_corner_velocity, float max_acceleration);
    static float centripetal_velocity_limit(float move_distance, float acceleration, float cos_half_theta);
};

} // namespace Slic3r::Boss
