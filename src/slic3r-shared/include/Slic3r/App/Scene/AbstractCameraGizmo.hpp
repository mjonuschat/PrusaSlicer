#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Render/DynamicGeometry.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Workbench.hpp"

#ifndef CAMERA_GIZMO_DEBUG
#define CAMERA_GIZMO_DEBUG 0
#endif

namespace Slic3r::App::Scene {

class AbstractCameraGizmo : public IGizmo,
                            public Biz::ISelectedBedInstancesChangedListener
{
public:
    enum class State : uint8_t {
        Inactive,
        Panning,
        Rotating
    };

    AbstractCameraGizmo(const Domain::Workbench& workbench, ISceneProvider& scene_provider)
      : m_workbench(workbench), m_scene_provider(scene_provider)
#if CAMERA_GIZMO_DEBUG
        , m_dynamic_geometry(Render::Context::instance().device())
#endif
    {}
    virtual ~AbstractCameraGizmo() = default;


    void register_commands(Platform::CommandRegistry& registry) override;
    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
#if CAMERA_GIZMO_DEBUG
    void render_scene(Render::CommandBuffer& cmd_buffer) override;
#endif

    /**
     * @name Implementation of Slic3r::Biz::ISelectedBedInstancesChangedListener public interface
     * @{
     */
    void on_selected_bed_instances_changed(Domain::SelectionId project_id, const Biz::Scene::BedSelection& selection) override;
    /**@}*/

private:
    void update_pan(const Domain::Vec3d& delta, bool synchronize_cam_pivot);

    /**
     * @param delta_x percentage of screen traversed in x direction
     * @param delta_y percentage of screen traversed in y direction
     * @param delta_for_180_rotation The percentage of the screen that
     * must be traversed to rotate the camera by 180 degrees (PI radians).
     * This effectively adjusts the camera sensitivity.
     */
    void update_rotation(float delta_x, float delta_y, float delta_for_180_rotation);
    void update_zoom(float wheel_delta_y);

    void look_at(const Domain::Vec3d& pos, double azimuth, double zenith);

    bool pick_plane(double mouse_x, double mouse_y, const Render::ScreenInfo& screen_info, Domain::Vec3d& out_plane_point);

    void center_camera_on_selected_bed();

private:
    virtual bool any_draggable(GizmoEventContext& ctx) const = 0;

private:
    const Domain::Workbench& m_workbench;

    ISceneProvider& m_scene_provider;
    State m_state{State::Inactive};
    float m_last_x{0.0f};
    float m_last_y{0.0f};
    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    Domain::BedRef m_selected_bed{Domain::INVALID_ID, Domain::INVALID_ID};
#if CAMERA_GIZMO_DEBUG
    Render::DynamicGeometry<Render::VertexP3> m_dynamic_geometry;
#endif // CAMERA_GIZMO_DEBUG
};

} // namespace Slic3r::App::Scene
