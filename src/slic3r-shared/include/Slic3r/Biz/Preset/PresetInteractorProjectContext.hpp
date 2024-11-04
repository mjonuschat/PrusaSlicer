#pragma once

#include <unordered_map>
#include "Slic3r/Domain/SelectionId.hpp"
#include "PresetInteractorConfigContainerContext.hpp"

namespace Slic3r::Domain { class Project; }

namespace Slic3r::Biz::Preset {

struct PresetInteractorProjectContext
{
    using ConfigContainerContexts = std::unordered_map<Domain::SelectionId, PresetInteractorConfigContainerContext>;

    Domain::SelectionId project_id;
    Domain::SelectionId selected_config_container_id;
    // TODO: Selected Object / Volume with ModelConfigObject
    ConfigContainerContexts  config_containers;
};

}
