#pragma once

#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/CustomGCode.hpp"
#include "Slic3r/Domain/Forward.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Transformation.hpp"

namespace Slic3r::Domain {

// TODO: move this to better place
using ModelInstanceList = std::vector<ModelInstance*>;

class Bed;

struct BedInstance : public ObjectBase
{
    explicit BedInstance(const Bed& bed) : bed(bed) {}

    const Transform3d& matrix() const
    {
        return transformation.get_matrix();
    }

    bool contains(const BoundingBox2d& bounds) const
    {
        Vec3d pos = transformation.get_offset();
        return bed.contains(Vec2d{pos.x(), pos.y()}, bounds);
    }

    std::string label() const
    {
        return std::to_string(index);
    }

    const Bed& bed;
    size_t index{0};
    Transformation transformation;
    ModelInstanceList model_instances;
    bool print_volume_enabled{false};
    std::optional<ModelWipeTower> wipe_tower;
    std::optional<CustomGCode::Info> custom_gcode;
};

} // namespace Slic3r::Domain
