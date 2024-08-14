#pragma once

#include <vector>
#include "PresetInteractorConfigContainerContext.hpp"

namespace Slic3r::Domain { class Project; }

namespace Slic3r::Biz {

struct PresetInteractorProjectContext
{
    using ConfigContainerContexts = std::vector<PresetInteractorConfigContainerContext>;
    Domain::Project& project;
    ConfigContainerContexts  config_containers;
};

}
