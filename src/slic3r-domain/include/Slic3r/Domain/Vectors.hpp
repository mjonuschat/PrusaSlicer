#pragma once
#include <Eigen/Dense>

namespace Slic3r::Domain {

/**@typedef coord_t
 * @brief Value 1.0/SCALING_FACTOR corresponds to 1mm. */
using coord_t = int32_t;

/**@typedef Vec2crd
 * @brief Vec2crd{1.0/SCALING_FACTOR, 1.0/SCALING_FACTOR} corresponds to (1mm, 1mm). */
using Vec2crd = Eigen::Matrix<coord_t, 2, 1, Eigen::DontAlign>;

/**@typedef Vec3crd
 * @brief See Vec2crd. */
using Vec3crd = Eigen::Matrix<coord_t, 3, 1, Eigen::DontAlign>;

/**@typedef Vec2big
 * @brief Can hold result of Vec2crd arithmetic operations. */
using Vec2big = Eigen::Matrix<int64_t, 2, 1, Eigen::DontAlign>;

using Vec2f = Eigen::Matrix<float, 2, 1, Eigen::DontAlign>;
using Vec3f = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
using Vec4f = Eigen::Matrix<float, 4, 1, Eigen::DontAlign>;
using Vec2d = Eigen::Matrix<double, 2, 1, Eigen::DontAlign>;
using Vec3d = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;
using Vec4d = Eigen::Matrix<double, 4, 1, Eigen::DontAlign>;

using Index2 = std::array<int, 2>;
using Index3 = std::array<int, 3>;
}
