#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::App::Plater {

struct SceneNodeTag {
    const Domain::SelectionId object_id{0};
    const Domain::SelectionId volume_id{0};
    const Domain::SelectionId instance_id{0};
};

}
