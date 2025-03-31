#pragma once

#include <memory>

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"
#include "Slic3r/App/Plater/SidebarSlice.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Plater/PlaterIFDMResultCacheChangedListener.hpp"

namespace Slic3r::App::Plater {

class PlaterRenderModule final : public Platform::AbstractRenderModule {
public:
    PlaterRenderModule(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
        : m_workbench(workbench)
        , m_project_interactor(project_interactor)
        , m_fdm_result_cache_changed_listener(std::bind(&PlaterRenderModule::on_fdm_cache_changed, this), m_project_interactor.fdm_result_cache())
    {
    }

    void render_scene() override;
    void render_imgui() override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;

protected:
    void on_init(Render::Device& device) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    void on_set_imgui_render() override;

private:
    void init_scene();
    void init_scene_layout();
    void render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);

    void init_gizmos();

    void on_fdm_cache_changed();

     /**
     * @brief Passes User-selected path to export to m_project_interactor.
     */
    void do_export();

    /**
     * @brief Passes User-selected path to upload to m_project_interactor.
     */
    void do_upload();

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PlaterScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;
    PlaterFDMResultCacheChangedListener m_fdm_result_cache_changed_listener;

    bool m_gui_win_open{true};

    // main window layout
    PlaterRenderLayout m_layout;
    SidebarSlice       m_sidebar_slice_panel;
};

} // namespace Slic3r::App::Plater
