#pragma once

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Render/DynamicGeometry.hpp"

#ifndef CAMERA_GIZMO_DEBUG
#define CAMERA_GIZMO_DEBUG 0
#endif

namespace Slic3r::App::Plater {

class CameraGizmo : public IGizmo {
public:
    enum class State : uint8_t {
        Inactive,
        Panning,
        Rotating
    };

    explicit CameraGizmo(ISceneProvider& scene_provider)
        : m_scene_provider(scene_provider)
#if CAMERA_GIZMO_DEBUG
        , m_dynamic_geometry(Render::Context::instance().device())
#endif
    {}

    void register_commands(CommandRegistry& registry) override;
    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
#if CAMERA_GIZMO_DEBUG
    void render_scene(Render::CommandBuffer& cmd_buffer) override;
#endif
private:
    void update_pan(const Vec3d& delta);
    void update_rotation(float delta_x, float delta_y);
    void update_zoom(float wheel_delta_y);

    void look_at(const Vec3d& pos, double azimuth, double zenith);

    bool pick_plane(double mouse_x, double mouse_y, const Render::ScreenInfo& screen_info, Vec3d& out_plane_point);
private:
    ISceneProvider& m_scene_provider;
    State m_state{State::Inactive};
    float m_last_x{0};
    float m_last_y{0};
#if CAMERA_GIZMO_DEBUG
    Render::DynamicGeometry<Render::VertexP3> m_dynamic_geometry;
#endif
};

}
