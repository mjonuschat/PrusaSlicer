#pragma once

#include <memory>

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Plater/GizmoManager.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#

#include "Slic3r/App/TestRenderLayout.hpp"

namespace Slic3r::App::Plater {

class PlaterRenderModule final : public Platform::AbstractRenderModule {
public:
    explicit PlaterRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench), m_project_interactor(project_interactor)
    {}

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
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<ScenePresenter> m_scene_presenter;
    //std::unique_ptr<Scene::Scene> m_scene;
    std::unique_ptr<GizmoManager> m_gizmo_manager;

    bool m_gui_win_open{true};

    // main window layout
    TestRenderLayout trl;
};

}
