#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Render/DynamicGeometry.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"

#ifndef CAMERA_GIZMO_DEBUG
#define CAMERA_GIZMO_DEBUG 0
#endif

namespace Slic3r::App::Scene {

class AbstractCameraGizmo : public IGizmo,
                            public Biz::ISelectedBedInstanceChangedListener
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
     * @name Implementation of Slic3r::Biz::ISelectedBedInstanceChangedListener public interface
     * @{
     */
    void on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id) override;
    /**@}*/

private:
    void update_pan(const Vec3d& delta, bool synchronize_cam_pivot);
    void update_rotation(float delta_x, float delta_y);
    void update_zoom(float wheel_delta_y);

    void look_at(const Vec3d& pos, double azimuth, double zenith);

    bool pick_plane(double mouse_x, double mouse_y, const Render::ScreenInfo& screen_info, Vec3d& out_plane_point);

private:
    virtual bool any_draggable(GizmoEventContext& ctx) const = 0;

private:
    const Domain::Workbench& m_workbench;

    ISceneProvider& m_scene_provider;
    State m_state{State::Inactive};
    float m_last_x{0};
    float m_last_y{0};
    Vec3d m_selected_bed_center{ Vec3d::Zero() };
#if CAMERA_GIZMO_DEBUG
    Render::DynamicGeometry<Render::VertexP3> m_dynamic_geometry;
#endif
};

} // namespace Slic3r::App::Scene
