#pragma once

#include <memory>

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Plater/GizmoManager.hpp"

namespace Slic3r::App::Plater {

class PlaterRenderModule final : public Platform::AbstractRenderModule {
public:
    void render_scene() override;
    void render_imgui() override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;

protected:
    void on_init(Render::Device& device) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;

private:
    void init_scene();
    void render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);

    void init_gizmos();
private:
    std::unique_ptr<Scene::Scene> m_scene;
    std::unique_ptr<GizmoManager> m_gizmo_manager;

    bool m_gui_win_open{true};
};

}
