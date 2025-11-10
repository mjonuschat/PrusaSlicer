#include "Slic3r/App/Scene/GizmoEventContext.hpp"

#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Plater/RotationGizmo.hpp"

namespace Slic3r::App::Scene {

bool GizmoEventContext::pick_results_contains_gizmo_nodes() const
{
    auto gizmo_it =
        std::find_if(m_pick_results.begin(), m_pick_results.end(), [&](const auto& item) {
        const Node& n = *item.node;
        return n.has_tag_of_type<Plater::GizmoNodeTag>() || 
               n.has_tag_of_type<Plater::RotationGizmoNodeTag>();
    });

    return gizmo_it != m_pick_results.end();
}

} // namespace Slic3r::App::Scene

