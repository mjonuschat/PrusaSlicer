#include "Slic3r/Biz/libpgcode/boss/KlipperCorneringModel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Slic3r::Boss {

float KlipperCorneringModel::junction_deviation_from_scv(float square_corner_velocity, float max_acceleration)
{
    if (square_corner_velocity <= 0.f || max_acceleration <= 0.f)
        return 0.f;
    return square_corner_velocity * square_corner_velocity * kSqrt2Minus1 / max_acceleration;
}

float KlipperCorneringModel::centripetal_velocity_limit(float move_distance, float acceleration, float cos_half_theta)
{
    if (cos_half_theta <= 0.f)
        return std::numeric_limits<float>::max();
    float sin_half_theta = std::sqrt(std::max(0.f, 1.f - cos_half_theta * cos_half_theta));
    float tan_half_theta = sin_half_theta / cos_half_theta;
    return std::sqrt(0.5f * move_distance * acceleration * tan_half_theta);
}

} // namespace Slic3r::Boss
