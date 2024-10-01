#pragma once

#include <cstddef>
#include <optional>

namespace Slic3r::App::Scene {
struct ModelInfo
{
    size_t object_id {0};
    size_t volume_id {0};
    size_t instance_id {0};
};

struct GizmoInfo
{
    size_t gizmo_id;
};

struct NodeInfo
{
    std::optional<ModelInfo> model_info;
    std::optional<GizmoInfo> gizmo_info;
};

}
