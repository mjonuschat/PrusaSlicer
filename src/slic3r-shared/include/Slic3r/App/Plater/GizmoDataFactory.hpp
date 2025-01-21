#pragma once

#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App::Plater {

static const ColorRGBA RED   = { 1.0f, 0.2f, 0.2f, 1.0f };
static const ColorRGBA GREEN = { 0.2f, 1.0f, 0.2f, 1.0f };
static const ColorRGBA BLUE  = { 0.2f, 0.2f, 1.0f, 1.0f };
static const ColorRGBA WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };

constexpr uint8_t CIRCLE_COARSE_GRADE_STEPS = 8; // 8 steps = 45 degrees step angle
constexpr uint8_t CIRCLE_FINE_GRADE_SECONDARY_STEPS = 72; // 72 steps = 5 degrees step angle
constexpr float CIRCLE_FINE_GRADE_PRIMARY_OUT_RADIUS = 1.1f;
constexpr float CIRCLE_FINE_GRADE_SECONDARY_OUT_RADIUS = 1.05f;
constexpr float CIRCLE_COARSE_GRADE_IN_RADIUS = 1.0f / 3.0f;
constexpr float CIRCLE_COARSE_GRADE_OUT_RADIUS = 2.0f / 3.0f;

enum class GizmoDataId
{
    ConeHandle = 0,
    AxesLines = 1,
    Segment = 2,
    Cone = 3,
    Cube = 4,
    Circle = 5,
    GradedCircle = 6,
};

enum class GizmoDataVariant
{
    None = 0,
    Red = 1,
    Green = 2,
    Blue = 3,
};

enum class GizmoDataTransform
{
    None = 0,
    PointX = 1,
    PointY = 2,
    PointZ = 3
};

class GizmoDataFactory
{
public:
    using TriangleMeshManager = Scene::TriangleMeshManager<GizmoDataId>;
    using GeometryManager = Render::GeometryManager<GizmoDataId>;

    explicit GizmoDataFactory(Render::Device& device) : m_device(device) {}

    /**
     * @brief Create new Node for given gizmo data, material variant and transform modification.
     *
     * @note The returned node is not inserted into scene, it's up to you to add the node into same
     * @p scene.
     *
     * @param scene A parent scene the node will be inserted into.
     * @param data_id ID of the gizmo data to create node for
     * @param data_variant A material variant (if needed)
     * @param data_transform A transformation type to be applied (if needed)
     * @return Node with drawable component (always), AabbRaycast component (optional), modified
     * transform (optional) and screen-sizing modifier (keeping constant screen size independently
     * on camera fov and distance).
     */
    std::unique_ptr<Scene::Node> create_node(
        Scene::Scene& scene,
        GizmoDataId data_id,
        GizmoDataVariant data_variant = GizmoDataVariant::None,
        GizmoDataTransform data_transform = GizmoDataTransform::None
    );

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
    void create_axes_lines();
    void create_segment();
    void create_circle();
    void create_graded_circle();

private:
    Render::Device& m_device;
    TriangleMeshManager m_triangle_mesh_manager;
    GeometryManager m_geometry_manager;
};

} // namespace Slic3r::App::Plater
