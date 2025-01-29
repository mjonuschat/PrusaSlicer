#pragma once

#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App::Plater {

constexpr uint8_t CIRCLE_COARSE_GRADE_STEPS = 8; // 8 steps = 45 degrees step angle
constexpr uint8_t CIRCLE_FINE_GRADE_SECONDARY_STEPS = 72; // 72 steps = 5 degrees step angle
constexpr float CIRCLE_FINE_GRADE_PRIMARY_OUT_RADIUS = 1.1f;
constexpr float CIRCLE_FINE_GRADE_SECONDARY_OUT_RADIUS = 1.05f;
constexpr float CIRCLE_COARSE_GRADE_IN_RADIUS = 1.0f / 3.0f;
constexpr float CIRCLE_COARSE_GRADE_OUT_RADIUS = 2.0f / 3.0f;

enum class GizmoDataId
{
    Segment = 0,
    Cone = 1,
    Cube = 2,
    Circle = 3,
    GradedCircle = 4,
};

class GizmoDataFactory
{
public:
    using TriangleMeshManager = Scene::TriangleMeshManager<GizmoDataId>;
    using GeometryManager = Render::GeometryManager<GizmoDataId>;

    explicit GizmoDataFactory(Render::Device& device) : m_device(device) {}

    /**
     * @brief Get or create triangle mesh for given gizmo data.
     *
     * @note The return value may be `nullptr` as not all gizmo data has triangle mesh (collisions).
     *
     * @param id Gizmo data identifier to get triangle mesh for
     * @return A pointer to Scene::TriangleMesh instance or `nullptr` if there is no such for given
     * gizmo data.
     */
    const Scene::TriangleMesh* triangle_mesh(GizmoDataId id)
    {
        auto triangle_mesh = m_triangle_mesh_manager.get(id);
        if (triangle_mesh == nullptr && geometry(id) == nullptr) {
            create_data(id);
            triangle_mesh = m_triangle_mesh_manager.get(id);
        }
        return triangle_mesh;
    }

    /**
     * @brief Get or create rendering geometry for given gizmo data @p id.
     *
     * @note Unlike the triangle_mesh() method, this will always return valid pointer to geometry as
     * every gizmo data has drawable object.
     *
     * @param id Gizmo data identifier to get geometry for.
     * @return Render::Geometry instance pointer.
     */
    const Render::Geometry* geometry(GizmoDataId id)
    {
        auto geometry = m_geometry_manager.get(id);
        if (geometry == nullptr) {
            create_data(id);
            geometry = DEBUG_ASSERT_VAL(m_geometry_manager.get(id));
        }
        return geometry;
    }

private:

    void create_data(GizmoDataId id);
    void create_segment();
    void create_circle();
    void create_graded_circle();

private:
    Render::Device& m_device;
    TriangleMeshManager m_triangle_mesh_manager;
    GeometryManager m_geometry_manager;
};

} // namespace Slic3r::App::Plater
