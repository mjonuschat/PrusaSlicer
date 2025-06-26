#pragma once

#include "Slic3r/Domain/Types.hpp"

#include <cereal/cereal.hpp>

namespace cereal {
template<class Archive>
void serialize(Archive& archive, Slic3r::Domain::Vec2crd& v)
{
    archive(v.x(), v.y());
}

template<class Archive>
void serialize(Archive& archive, Slic3r::Domain::Vec2f& v)
{
    archive(v.x(), v.y());
}

template<class Archive>
void serialize(Archive& archive, Slic3r::Domain::Vec3f& v)
{
    archive(v.x(), v.y(), v.z());
}

template<class Archive>
void serialize(Archive& archive, Slic3r::Domain::Vec2d& v)
{
    archive(v.x(), v.y());
}

template<class Archive>
void serialize(Archive& archive, Slic3r::Domain::Vec3d& v)
{
    archive(v.x(), v.y(), v.z());
}
} // namespace cereal
