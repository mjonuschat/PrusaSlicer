#pragma once

#include "Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Domain { class Project; }


namespace Slic3r::Biz::Scene {

struct SceneInteractorProjectContext
{
    Domain::Project& project;
    Selection selection;
};

}
