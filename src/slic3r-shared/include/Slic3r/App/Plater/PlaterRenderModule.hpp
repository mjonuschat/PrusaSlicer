#pragma once

#include <memory>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/DialogNavigation.hpp"
#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Scene/ModelGeometryProvider.hpp"

namespace Slic3r::Biz {
class ThumbnailImageProvider;
} // namespace Slic3r::Biz

namespace Slic3r::App {
struct ThumbnailStore;
class ThumbnailStoreUpdater;
class Navigator;
class ObjectListWindow;
class SidebarBed;
class SidebarObject;
class SidebarPrint;
class CubeView;
class TopBar;
class PreferencesDialog;
} // namespace Slic3r::App

namespace Slic3r::App::Yoga {
class Menu;
class ToolbarButton;
class ToolbarSwitchButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::PopNotification {
class PopNotificationListView;
} // namespace Slic3r::App::PopNotification

namespace Slic3r::App::Plater {
class TranslationGizmo;
class RotationGizmo;
class ScaleGizmo;
class PlaceOnFaceGizmo;
class PaintOnSupportsGizmo;
class PaintOnSeamsGizmo;
class PaintOnFuzzySkinGizmo;
class MultiMaterialPaintingGizmo;
class SimplifyGizmo;
class TextGizmo;
class MeasureGizmo;
class PlaterCameraGizmo;
class CutGizmo;
class VariableLayerHeightGizmo;
class HeightRangeGizmo;
class ArrangeGizmo;
class PlaterScenePresenter;
class PlaterRenderLayout;
class SidebarPlaterActionButtons;
class History;
class ThumbnailImageGenerator;

class PlaterRenderModule final :
    public Platform::AbstractRenderModule,
    public Biz::IStatusCacheChangedListener,
    public Biz::Scene::ISceneSelectionChangedListener,
    private Scene::IGizmoActiveToolListener,
    public Biz::ISelectedProjectChangedListener,
    public Biz::Preset::IPresetChangedListener,
    public Scene::ISharedModelGeometryProvider
{
public:
    PlaterRenderModule(
        const Domain::Workbench& workbench,
        Biz::ProjectInteractor& project_interactor,
        std::shared_ptr<ThumbnailStore> thumbnail_store,
        std::shared_ptr<ThumbnailStoreUpdater> thumbnail_store_updater,
        std::shared_ptr<ThumbnailImageGenerator> thumbnail_image_generator,
        std::unique_ptr<Biz::Emboss::IFontManager> font_manager
    );
    ~PlaterRenderModule();

    void render_scene(Render::CommandBuffer& cmd_buffer) override;
    void render_imgui(Render::CommandBuffer& cmd_buffer) override;
    void on_scene_mouse_event(const Platform::MouseEvent& e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent& e) override;
    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    ) override;
    void set_navigator(Navigator* navigator) override;

    void on_status_cache_status_code_changed(const Domain::SlicingId id) override;

    void set_sidebars_visible(bool visible) override;

    const std::optional<Platform::CameraSynchData>& camera_synch_data() const override;
    void set_camera_synch_data(const Platform::CameraSynchData& data) override;

    void set_opened_dialog(Yoga::Dialog* opened_dialog);

    void navigate_to_item(const Domain::ConfigItem* config_item);

    void open_search();
    void set_opened_preferences(bool opened);
    bool is_opened_preferences();

    void set_object_list_collapsed(bool collapsed);

    MenuManager& menu_manager() override
    {
        return m_menu_manager;
    }

    CommandBindingManager& command_binding_manager() override
    {
        return m_command_binding_manager;
    }

    const Platform::CommandRegistry::CommandsMap& gizmo_commands() const override
    {
        ASSERT(m_gizmo_manager);
        return m_gizmo_manager->commands();
    }

    const Platform::ICommand& command(const char* name) const override
    {
        if (gizmo_commands().contains(name)) {
            return m_gizmo_manager->command(name);
        }
        return m_command_registry.command(name);
    }

    bool is_gizmo_manager_completed() const override
    {
        return m_gizmo_manager ? true : false;
    }

    /**
     * @name Implementation of Scene::ISharedModelGeometryProvider public interface
     * @{
     */
    std::shared_ptr<Scene::ModelGeometryProvider> shared_model_geometry_provider() override;
    /**@}*/

protected:
    void on_init(Render::Device& device, Render::ImguiRender& imgui_render, Platform::AnimationManager& animation_manager) override;
    void on_activated() override;
    void on_deactivated() override;
    void on_screen_resized() override;
    void on_set_imgui_render() override;
    void register_commands() override;
    void bind_commands() override;
    /**
     * @name Implementation of Biz::ISelectedProjectChangedListener public interface
     * @{
     */
    void on_selected_project_changed(size_t index) override;
    /**@}*/

    void on_preset_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::PresetItemType type
    ) override;

private:
    void active_tool_changed(Scene::IToolGizmo* active_tool) override;

    void init_scene();
    void init_scene_layout();
    void update_tool_selection(Scene::ToolType current_tool_type);
    void
    render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);
    void toggle_activate_tool(Scene::ToolType tool_type);
    void init_dialog_navigation();
    void update_object_selection();
    void update_current_right_sidebar();
    void update_toolbar_visibility();

    void init_gizmos();
    void init_add_volume_menu(Yoga::Item* parent);
    void add_volume(const Domain::ModelVolumeType& type);

    Yoga::ToolbarButton* get_toolbar_button(Scene::ToolType tool_type) const;

private:
    const Domain::Workbench& m_workbench;
    Biz::ProjectInteractor& m_project_interactor;
    std::unique_ptr<PlaterScenePresenter> m_scene_presenter;
    std::unique_ptr<Scene::GizmoManager> m_gizmo_manager;

    // tmp menu for add volume
    Yoga::Menu* m_add_volumes_menu = nullptr;
    // main window layout
    std::unique_ptr<PlaterRenderLayout> m_layout;
    // Layout objects
    Yoga::Passthrough<TopBar> m_top_bar;
    Yoga::Passthrough<ObjectListWindow> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<PopNotification::PopNotificationListView> m_pop_notification_list_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
    Yoga::Passthrough<SidebarObject> m_sidebar_object;
    Yoga::Passthrough<SidebarPlaterActionButtons> m_sidebar_action_buttons;
    Yoga::Passthrough<History> m_history;
    Yoga::Passthrough<PreferencesDialog> m_preferences_dialog;

    Yoga::ToolbarButton* m_toolbar_add                     = nullptr;
    Yoga::ToolbarButton* m_toolbar_add_volume              = nullptr;
    Yoga::ToolbarButton* m_toolbar_delete                  = nullptr;
    Yoga::ToolbarButton* m_toolbar_add_instance            = nullptr;
    Yoga::ToolbarButton* m_toolbar_move                    = nullptr;
    Yoga::ToolbarButton* m_toolbar_rotate                  = nullptr;
    Yoga::ToolbarButton* m_toolbar_scale                   = nullptr;
    Yoga::ToolbarButton* m_toolbar_place_on_face           = nullptr;
    Yoga::ToolbarButton* m_toolbar_simplify                = nullptr;
    Yoga::ToolbarButton* m_toolbar_arrange                 = nullptr;
    Yoga::ToolbarButton* m_toolbar_paint_on_supports       = nullptr;
    Yoga::ToolbarButton* m_toolbar_paint_on_seams          = nullptr;
    Yoga::ToolbarButton* m_toolbar_paint_on_fuzzy_skin     = nullptr;
    Yoga::ToolbarButton* m_toolbar_multi_material_painting = nullptr;
    Yoga::ToolbarButton* m_toolbar_text                    = nullptr;
    Yoga::ToolbarButton* m_toolbar_measure                 = nullptr;
    Yoga::ToolbarButton* m_toolbar_cut                     = nullptr;
    Yoga::ToolbarButton* m_toolbar_variable_layer_height   = nullptr;
    Yoga::ToolbarButton* m_toolbar_height_range            = nullptr;
    Yoga::ToolbarSwitchButton* m_toolbar_preview_switch    = nullptr;

    std::unique_ptr<Biz::Emboss::IFontManager> m_font_manager = nullptr;

    TranslationGizmo* m_translation_gizmo                       = nullptr;
    RotationGizmo* m_rotation_gizmo                             = nullptr;
    ScaleGizmo* m_scale_gizmo                                   = nullptr;
    PlaceOnFaceGizmo* m_place_on_face_gizmo                     = nullptr;
    ArrangeGizmo* m_arrange_gizmo                               = nullptr;
    SimplifyGizmo* m_simplify_gizmo                             = nullptr;
    PaintOnSupportsGizmo* m_paint_on_supports_gizmo             = nullptr;
    PaintOnSeamsGizmo* m_paint_on_seams_gizmo                   = nullptr;
    PaintOnFuzzySkinGizmo* m_paint_on_fuzzy_skin_gizmo          = nullptr;
    MultiMaterialPaintingGizmo* m_multi_material_painting_gizmo = nullptr;
    TextGizmo* m_text_gizmo                                     = nullptr;
    MeasureGizmo* m_measure_gizmo                               = nullptr;
    PlaterCameraGizmo* m_camera_gizmo                           = nullptr;
    CutGizmo* m_cut_gizmo                                       = nullptr;
    VariableLayerHeightGizmo* m_variable_layer_height_gizmo     = nullptr;
    HeightRangeGizmo* m_height_range_gizmo                      = nullptr;

    std::shared_ptr<ThumbnailStore> m_thumbnail_store;
    std::shared_ptr<ThumbnailStoreUpdater> m_thumbnail_store_updater;
    std::shared_ptr<Plater::ThumbnailImageGenerator> m_thumbnail_image_generator;

    Navigator* m_render_module_navigator{nullptr};

    DialogNavigation m_dialog_navigation;
    MenuManager m_menu_manager;
    CommandBindingManager m_command_binding_manager;

    std::set<Yoga::Dialog*> m_gizmo_dialogs;
};

} // namespace Slic3r::App::Plater
