#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include <libassert/assert.hpp>

namespace Slic3r::Biz::Algorithms::BoundingBox {

[[nodiscard]] auto construct(const std::input_iterator auto from, const std::input_iterator auto to)
{
    using PointType = std::remove_reference_t<decltype(*from)>;
    using ResultType = Domain::BoundingBox<
        typename PointType::Scalar,
        PointType::RowsAtCompileTime
    >;

    if (from == to) {
        return ResultType{};
    }

    auto it{from};
    auto min{*it};
    auto max{*it};
    for (++it; it != to; ++it) {
        min = min.cwiseMin(*it);
        max = max.cwiseMax(*it);
    }

    return ResultType{min, max};
}

[[nodiscard]] auto construct(const std::ranges::range auto& points)
{
    return construct(std::begin(points), std::end(points));
}


namespace Impl {
template <typename T>
constexpr bool is_floating = std::is_same_v<typename T::Scalar, float> || std::is_same_v<typename T::Scalar, double>;
}

template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] std::enable_if_t<Impl::is_floating<BoxType>, bool> approx_equals(
    const BoxType& a, const BoxType& b
)
{
    ASSERT(a.defined && b.defined);
    return a.min.isApprox(b.min) && a.max.isApprox(b.max);
}

template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] double bbox_point_distance(const BoxType &box, const typename BoxType::VecType &point) {
    ASSERT(box.defined);

    Eigen::AlignedBox<typename BoxType::Scalar, BoxType::Dim> eigen_box{box.min, box.max};
    return eigen_box.exteriorDistance(point);
}

template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] double bbox_point_distance_squared(const BoxType &box, const typename BoxType::VecType &point) {
    ASSERT(box.defined);

    Eigen::AlignedBox<typename BoxType::Scalar, BoxType::Dim> eigen_box{box.min, box.max};
    return eigen_box.squaredExteriorDistance(point);
}


template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] auto merge(const BoxType& a, const BoxType& b) {
    ASSERT(b.defined || b.min.isApprox(b.max));
    if (a.defined) {
        return BoxType{a.min.cwiseMin(b.min), a.max.cwiseMax(b.max)};
    } else {
        BoxType result{b};
        result.defined = true;
        return result;
    }
}

template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] auto merge(const BoxType& a, const typename BoxType::VecType& point) {
    if (a.defined) {
        return BoxType{a.min.cwiseMin(point), a.max.cwiseMax(point)};
    } else {
        return BoxType{point, point, true};
    }
}

template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] auto inflated(
    const BoxType& box, const typename BoxType::Scalar& delta
)
{
    ASSERT(box.defined);

    using Vec = typename BoxType::VecType;
    const Vec vector_delta{Vec::Ones() * delta};

    auto result{box};
    result.min -= vector_delta;
    result.max += vector_delta;

    return result;
}

template<Domain::ScaledScalar OutputScalarType = Domain::coord_t, Domain::BoundingBoxConcept InputBoxType>
[[nodiscard]] auto scaled(const InputBoxType& box)
{
    ASSERT(box.defined);

    return Domain::BoundingBox<OutputScalarType, InputBoxType::Dim>{
        Scaling::scaled<OutputScalarType>(box.min),
        Scaling::scaled<OutputScalarType>(box.max)
    };
}

template<typename OutputScalarType = float, Domain::BoundingBoxConcept InputBoxType>
[[nodiscard]] auto unscaled(const InputBoxType& box)
{
    ASSERT(box.defined);

    return Domain::BoundingBox<OutputScalarType, InputBoxType::Dim>{
        Scaling::unscaled<OutputScalarType>(box.min),
        Scaling::unscaled<OutputScalarType>(box.max),
    };
}

template<typename BoxType>
[[nodiscard]] auto to_2d(const BoxType& box)
{
    return Domain::BoundingBox<typename BoxType::Scalar, 2>{
        box.min.template head<2>(),
        box.max.template head<2>(),
        box.defined
    };
}

template<typename BoxType>
[[nodiscard]] BoxType translated(
    const BoxType& box,
    const Domain::Advanced::Vec<typename BoxType::Scalar, BoxType::Dim>& delta
) {
    ASSERT(box.defined);

    using Domain::Advanced::Transform;
    using Domain::Advanced::Translation;

    const Domain::Advanced::Transform<typename BoxType::Scalar, BoxType::Dim> translation{
        Domain::Advanced::Translation<typename BoxType::Scalar, BoxType::Dim>{delta}};

    return BoxType{translation * box.min, translation * box.max};
}

// TODO required polygon
/**
inline Domain::Polygon to_polygon(const Domain::BoundingBox2i32& box)
{
    return Domain::Polygon{{
        box.min,
        { box.max.x(), box.min.y() },
        box.max,
        { box.min.x(), box.max.y() }
    }};
}
*/

} // namespace Slic3r::Biz::Algorithms::BoundingBox

namespace cereal {
template<class Archive, Slic3r::Domain::BoundingBoxConcept BoxType>
void serialize(Archive& archive, const BoxType& box)
{
    archive(box.min, box.max, box.defined);
}

} // namespace cereal
