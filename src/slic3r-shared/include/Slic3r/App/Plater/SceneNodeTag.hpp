#pragma once

#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/ElementRef.hpp"

namespace Slic3r::App::Plater {

struct SceneNodeTag
{
    const Domain::SelectionId object_id{0};
    const Domain::SelectionId volume_id{0};
    const Domain::SelectionId instance_id{0};
    const Domain::ModelVolumeType volume_type{Domain::ModelVolumeType::INVALID};

    bool matches_element(const Domain::ElementRef& e) const
    {
        return e.object_id == object_id && e.instance_id == instance_id && e.volume_id == volume_id;
    }
};

} // namespace Slic3r::App::Plater
