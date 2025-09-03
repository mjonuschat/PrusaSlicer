#pragma once

#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/CustomGCode.hpp"
#include "Slic3r/Domain/Forward.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Transformation.hpp"

namespace Slic3r::Domain {

// TODO: move this to better place
using ModelInstanceList      = std::vector<ModelInstance*>;
using ConstModelInstanceList = std::vector<const ModelInstance*>;

class Bed;

struct BedInstance : public ObjectBase
{
    explicit BedInstance(const Bed& bed);

    const Transform3d& matrix() const
    {
        return transformation.get_matrix();
    }

    size_t index() const;
    void set_index(size_t index);

    const std::string& label() const;
    const std::string& name() const;

    std::reference_wrapper<const Bed> bed;
    Transformation transformation;
    ModelInstanceList model_instances;
    bool print_volume_enabled{false};
    std::optional<ModelWipeTower> wipe_tower;
    std::optional<CustomGCode::Info> custom_gcode;

private:
    size_t m_index{0};
    std::string m_label;
    std::string m_name;
};

} // namespace Slic3r::Domain
