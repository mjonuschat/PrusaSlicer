#pragma once

#include "Slic3r/Biz/SelectionId.hpp"

namespace Slic3r::App::Plater {

struct SceneNodeTag {
    const Biz::SelectionId object_id{0};
    const Biz::SelectionId volume_id{0};
    const Biz::SelectionId instance_id{0};
};

}
