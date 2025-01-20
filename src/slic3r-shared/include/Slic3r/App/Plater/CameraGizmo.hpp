#pragma once

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Plater {

class CameraGizmo : public IGizmo {
public:
    enum class State : uint8_t {
        Inactive,
        Panning,
        Rotating
    };

    explicit CameraGizmo(ISceneProvider& scene_provider) : m_scene_provider(scene_provider) {}

    void register_commands(CommandRegistry& registry) override;
    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
private:
    void update_pan(const Vec3d& delta);
    void update_rotation(float delta_x, float delta_y);
    void update_zoom(float wheel_delta_y);

    bool pick_plane(const GizmoEventContext& ctx, Vec3d& out_plane_point);
private:
    ISceneProvider& m_scene_provider;
    State m_state{State::Inactive};
    float m_last_x{0};
    float m_last_y{0};
    Vec3d m_mouse_last_world_position;
};

}
