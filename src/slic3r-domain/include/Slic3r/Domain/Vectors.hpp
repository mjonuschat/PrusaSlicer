#pragma once
#include <Eigen/Dense>

namespace Slic3r::Domain {

using coord_t = int32_t;

using Vec2crd = Eigen::Matrix<coord_t, 2, 1, Eigen::DontAlign>;
using Vec3crd = Eigen::Matrix<coord_t, 3, 1, Eigen::DontAlign>;

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
