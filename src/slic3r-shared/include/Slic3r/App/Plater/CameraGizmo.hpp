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

    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
private:
    void update_pan(float delta_x, float delta_y);
    void update_rotation(float delta_x, float delta_y);
    void update_zoom(float wheel_delta_y);
private:
    ISceneProvider& m_scene_provider;
    State m_state{State::Inactive};
    float m_last_x{0};
    float m_last_y{0};
};

}
