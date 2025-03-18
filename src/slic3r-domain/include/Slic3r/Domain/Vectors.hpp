#pragma once
#include <Eigen/Dense>

namespace Slic3r::Domain {

/**@typedef coord_t
 * @brief Value 1.0/SCALING_FACTOR corresponds to 1mm. */
using coord_t = int32_t;

namespace Advanced {
template<typename Scalar, std::size_t Dim>
using Vec = Eigen::Matrix<Scalar, Dim, 1, Eigen::DontAlign>;

template<typename Scalar, std::size_t Dim>
using Transform = Eigen::Transform<Scalar, Dim, Eigen::Affine, Eigen::DontAlign>;

template<typename Scalar, std::size_t Dim>
using Translation = Eigen::Translation<Scalar, Dim>;
} // namespace Advanced

/**@typedef Vec2crd
 * @brief Vec2crd{1.0/SCALING_FACTOR, 1.0/SCALING_FACTOR} corresponds to (1mm, 1mm). */
using Vec2crd = Advanced::Vec<coord_t, 2>;

/**@typedef Vec3crd
 * @brief See Vec2crd. */
using Vec3crd = Advanced::Vec<coord_t, 3>;

/**@typedef Vec2big
 * @brief Can hold result of Vec2crd arithmetic operations. */
using Vec2big = Advanced::Vec<int64_t, 2>;

/**@typedef Vec3big
 * @brief See Vec2big */
using Vec3big = Advanced::Vec<int64_t, 3>;

using Vec2f = Advanced::Vec<float, 2>;
using Vec3f = Advanced::Vec<float, 3>;
using Vec4f = Advanced::Vec<float, 4>;
using Vec2d = Advanced::Vec<double, 2>;
using Vec3d = Advanced::Vec<double, 3>;
using Vec4d = Advanced::Vec<double, 4>;

using Index2 = std::array<int, 2>;
using Index3 = std::array<int, 3>;

template<typename T>
concept ScaledScalar = (
    std::is_same_v<T, coord_t> || std::is_same_v<T, int64_t>
);

template<typename T>
concept UnscaledScalar = (
    std::is_same_v<T, float> || std::is_same_v<T, double>
);

template<typename T>
concept ScaledVector = (
    std::is_same_v<typename T::Scalar, coord_t> || std::is_same_v<typename T::Scalar, int64_t>
);

template<typename T>
concept UnscaledVector = (
    std::is_same_v<typename T::Scalar, float> || std::is_same_v<typename T::Scalar, double>
);

}
