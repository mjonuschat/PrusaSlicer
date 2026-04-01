#pragma once


#include "Slic3r/Biz/Scene/Selection.hpp"
namespace Slic3r::App::Undo {
struct BedSelectionState
{
    BedSelectionState() = default;
    BedSelectionState(const Biz::Scene::BedSelection& selection);

    Domain::BedRefs selected_beds;
    Domain::SelectionId selected_config_container{};
    Domain::BedRef last_selected_bed{};
    Biz::Scene::BedSelectionMode mode{};
    Biz::Scene::CameraActionOnBedSelection camera_action_on_selection{};
};
}
