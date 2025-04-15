#pragma once

#include <memory>

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"
#include "Slic3r/App/SidebarActionButtons.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Plater/PlaterIFDMResultCacheChangedListener.hpp"

namespace Slic3r::App::Plater {

class PlaterRenderModule final : public Platform::AbstractRenderModule,
                                 public Biz::IStatusCacheChangedListener
{
public:
    PlaterRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench)
        , m_project_interactor(project_interactor)
    {
    }

    void render_scene(Render::CommandBuffer& cmd_buffer) override;
    void render_imgui(Render::CommandBuffer& cmd_buffer) override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    void add_type_changed_listener(IRenderModuleChangedListener* l) override;
    void remove_type_changed_listener(IRenderModuleChangedListener* l) override;

    void on_status_cache_changed(
        const Biz::Slicing::SlicingId id
    ) override;

protected:
    void on_init(Render::Device& device, Render::ImguiRender& imgui_render) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    void on_set_imgui_render() override;

private:
    void init_scene();
    void init_scene_layout();
    void render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);

    void init_gizmos();

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PlaterScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;

    bool m_gui_win_open{true};

    // main window layout
    PlaterRenderLayout m_layout;
    SidebarActionButtons m_sidebar_actions_panel;
};

} // namespace Slic3r::App::Plater
