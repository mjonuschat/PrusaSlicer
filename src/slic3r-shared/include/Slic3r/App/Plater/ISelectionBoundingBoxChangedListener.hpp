#pragma once

#include <optional>
#include "Slic3r/App/Scene/OrientedBoundingBox.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::App::Scene {
class BedError;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

class ISelectionBoundingBoxChangedListener
{
public:
    virtual ~ISelectionBoundingBoxChangedListener() = default;

    virtual void on_scene_selection_bounding_box_changed(
        Domain::SelectionId project_id,
        const std::optional<Scene::OrientedBoundingBox>& bounding_box
    ) = 0;
};

} // namespace Slic3r::App::Plater
