#pragma once

#include "Slic3r/Biz/Scene/Selection.hpp"

namespace Slic3r::Domain { class Project; }


namespace Slic3r::Biz::Scene {

struct SceneInteractorProjectContext
{
    Domain::Project& project;
    ObjectSelection object_selection;
    BedSelection bed_selection;
};

}
