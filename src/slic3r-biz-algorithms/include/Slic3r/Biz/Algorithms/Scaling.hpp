#pragma once

#include "Slic3r/Domain/Point.hpp"
#include <cinttypes>

namespace Slic3r::Biz::Algorithms::Scaling {

static constexpr float SCALING_FACTOR = 0.000001;

template <Domain::ScaledScalar OutputType = Domain::coord_t>
inline constexpr OutputType scaled(const Domain::UnscaledScalar auto &v)
{
    using InputType = typename std::remove_reference_t<decltype(v)>;
    return static_cast<OutputType>(v / static_cast<InputType>(SCALING_FACTOR));
}

template <Domain::UnscaledScalar OutputType = float>
inline constexpr OutputType unscaled(const Domain::ScaledScalar auto &v)
{
    return static_cast<OutputType>(v) * static_cast<OutputType>(SCALING_FACTOR);
}

template <Domain::ScaledScalar OutputScalarType = Domain::coord_t>
inline constexpr Domain::ScaledVector auto scaled(const Domain::UnscaledVector auto &v)
{
    using InputType = typename std::remove_reference_t<decltype(v)>::Scalar;
    return (v / static_cast<InputType>(SCALING_FACTOR)).template cast<OutputScalarType>();
}

template <Domain::UnscaledScalar OutputScalarType = float>
inline constexpr Domain::UnscaledVector auto unscaled(const Domain::ScaledVector auto &v)
{
    return v.template cast<OutputScalarType>() * SCALING_FACTOR;
}

}
