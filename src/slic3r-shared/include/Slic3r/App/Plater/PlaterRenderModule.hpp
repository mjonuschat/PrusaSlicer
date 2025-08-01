#pragma once

#include <memory>

#include "Slic3r/App/Plater/ArrangeGizmo.hpp"
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"
#include "Slic3r/App/SharedThumbnailImageGenerator.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz {
class ThumbnailImageProvider;
} // namespace Slic3r::Biz

namespace Slic3r::App {
struct ThumbnailStore;
class ThumbnailStoreUpdater;
class Navigator;

namespace Yoga {
class Menu;
} // namespace Yoga

} // namespace Slic3r::App

namespace Slic3r::App::Plater {
class TranslationGizmo;
class RotationGizmo;
class PaintOnSupportsGizmo;
class SimplifyGizmo;
class TextGizmo;
class MeasureGizmo;

class PlaterRenderModule final :
    public Platform::AbstractRenderModule,
    public Biz::IStatusCacheChangedListener,
    public Biz::Scene::ISceneSelectionChangedListener,
    private Scene::IGizmoActiveToolListener
{
public:
    PlaterRenderModule(
        const Domain::Workbench& workbench,
        Biz::ProjectInteractor& project_interactor,
        Biz::ThumbnailImageProvider& thumbnail_image_provider,
        std::shared_ptr<ThumbnailStore> thumbnail_store,
        std::shared_ptr<ThumbnailStoreUpdater> thumbnail_store_updater,
        std::shared_ptr<App::SharedThumbnailImageGenerator> shared_thumbnail_image_generator
    );
    ~PlaterRenderModule();

    void render_scene(Render::CommandBuffer& cmd_buffer) override;
    void render_imgui(Render::CommandBuffer& cmd_buffer) override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection &selection) override;
    void set_navigator(Navigator* navigator) override;

    void on_status_cache_changed(
        const Biz::Slicing::SlicingId id
    ) override;

    void set_sidebars_visible(bool visible) override;

protected:
    void on_init(Render::Device& device, Render::ImguiRender& imgui_render) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    void on_set_imgui_render() override;

private:
    void init_scene();
    void init_scene_layout();
    void update_toolbar_tool_selection(Scene::ToolType current_tool_type);
    void render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);
    void toggle_activate_tool(Scene::ToolType tool_type);
    void active_tool_changed(Scene::IToolGizmo* active_tool) override;

    void init_gizmos();
    void init_add_volume_menu();
    void add_volume(const Domain::ModelVolumeType& type);

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PlaterScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;

    bool m_gui_win_open{true};

    // tmp menu for add volume
    std::unique_ptr<Yoga::Menu> m_add_volumes_menu;
    // main window layout
    std::unique_ptr<PlaterRenderLayout> m_layout;
    // Layout objects
    Yoga::Passthrough<TopBar> m_top_bar;
    Yoga::Passthrough<ObjectListWindow> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
    Yoga::Passthrough<SidebarPlaterActionButtons> m_sidebar_action_buttons;
    Yoga::Passthrough<History> m_history;

    Yoga::ToolbarButton* m_toolbar_add_volume        = nullptr;
    Yoga::ToolbarButton* m_toolbar_delete            = nullptr;
    Yoga::ToolbarButton* m_toolbar_add_instance      = nullptr;
    Yoga::ToolbarButton* m_toolbar_move              = nullptr;
    Yoga::ToolbarButton* m_toolbar_rotate            = nullptr;
    Yoga::ToolbarButton* m_toolbar_simplify          = nullptr;
    Yoga::ToolbarButton* m_toolbar_arrange           = nullptr;
    Yoga::ToolbarButton* m_toolbar_paint_on_supports = nullptr;
    Yoga::ToolbarButton* m_toolbar_text              = nullptr;
    Yoga::ToolbarButton* m_toolbar_measure           = nullptr;

    TranslationGizmo* m_translation_gizmo           = nullptr;
    RotationGizmo* m_rotation_gizmo                 = nullptr;
    ArrangeGizmo* m_arrange_gizmo                   = nullptr;
    SimplifyGizmo* m_simplify_gizmo                 = nullptr;
    PaintOnSupportsGizmo* m_paint_on_supports_gizmo = nullptr;
    TextGizmo* m_text_gizmo                         = nullptr;
    MeasureGizmo* m_measure_gizmo                   = nullptr;

    std::shared_ptr<App::SharedThumbnailImageGenerator> m_shared_thumbnail_image_generator;
    Biz::ThumbnailImageProvider& m_thumbnail_image_provider;

    std::shared_ptr<ThumbnailStore> m_thumbnail_store;
    std::shared_ptr<ThumbnailStoreUpdater> m_thumbnail_store_updater;

    Navigator* m_render_module_navigator{ nullptr };
};

} // namespace Slic3r::App::Plater
