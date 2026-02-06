#pragma once

#include "Slic3r/Biz/Scene/Selection.hpp"
#include "libslic3r/PrintBase.hpp"

namespace Slic3r::Domain { class Project; }


namespace Slic3r::Biz::Scene {

struct SceneInteractorProjectContext
{
    Domain::Project& project;
    BedSelection bed_selection;
    ObjectSelection object_selection;
    SelectionReferenceFrame object_selection_reference_frame{SelectionReferenceFrame::Bed};

    // key is bed_instance_id
    std::map<std::size_t, Biz::Print::WipeTowerGeometry> wipe_tower_geometries;
};

}
