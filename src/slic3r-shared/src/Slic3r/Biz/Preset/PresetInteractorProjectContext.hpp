#pragma once

#include <unordered_map>
#include "PresetInteractorConfigContainerContext.hpp"

namespace Slic3r::Domain { class Project; }

namespace Slic3r::Biz::Preset {

struct PresetInteractorProjectContext
{
    using ConfigContainerContexts = std::unordered_map<SelectionId, PresetInteractorConfigContainerContext>;
    SelectionId project_id;
    ConfigContainerContexts  config_containers;
};

}
