#include "Slic3r/App/Plater/PlaterRenderModule.hpp"
#include "Slic3r/App/Plater/ArrangeGizmo.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Scene/LightingHelper.hpp"
#include "Slic3r/App/Scene/MouseDragDetector.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/Domain/Image.hpp"
#include "Slic3r/App/Plater/PlaterCameraGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/App/Scene/BedRenderHelper.hpp"
#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/BedSelectGizmo.hpp"
#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/App/Plater/RotationGizmo.hpp"
#include "Slic3r/App/Plater/SimplifyGizmo.hpp"
#include "Slic3r/App/Plater/PaintOnSupportsGizmo.hpp"
#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"
#include "Slic3r/App/Plater/MeasureGizmo.hpp"
#include "Slic3r/App/Plater/MeasureDialog.hpp"
#include "Slic3r/App/Plater/TextGizmo.hpp"
#include "Slic3r/App/Plater/TextDialog.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/ThumbnailStoreUpdater.hpp"
#include "Slic3r/App/Scene/CameraHelper.hpp"

#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/SidebarObject.hpp"
#include "Slic3r/App/SidebarActionButtons.hpp"
#include "Slic3r/App/LightSetting.hpp"
#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"
#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/MenuItem.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/RenderModuleHelper.hpp"

#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/Math.hpp"

#include <imgui/imgui.h>
#include <Eigen/SVD>

#define ENABLED_DEBUG_BEDS 0

using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec4d;

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App::Plater {

namespace TriMesh = Biz::Algorithms::TriangleMesh;

PlaterRenderModule::PlaterRenderModule(
    const Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor,
    std::shared_ptr<ThumbnailStore> thumbnail_store,
    std::shared_ptr<ThumbnailStoreUpdater> thumbnail_store_updater,
    std::shared_ptr<Plater::ThumbnailImageGenerator> thumbnail_image_generator
) :
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_thumbnail_store(thumbnail_store),
    m_thumbnail_store_updater(thumbnail_store_updater),
    m_thumbnail_image_generator(thumbnail_image_generator)
{}

PlaterRenderModule::~PlaterRenderModule()
{
    if (m_gizmo_manager) {
        m_gizmo_manager->remove_listener<IGizmoActiveToolListener>(this);
    }
}

void PlaterRenderModule::set_opened_dialog(Yoga::Dialog* opened_dialog)
{
    if (!m_gizmo_dialogs.contains(opened_dialog)
        && m_gizmo_manager->current_tool_type() != Scene::ToolType::None)
    {
        m_gizmo_manager->deactivate_current_tool();
    }

    m_dialog_navigation.open_dialog(opened_dialog);
}

void PlaterRenderModule::on_init(Render::Device& device, Render::ImguiRender& imgui_render)
{
    AbstractRenderModule::on_init(device, imgui_render);
    Yoga::Item::set_imgui_render(&imgui_render); // Todo: move this somewhere where it is invoked once
    m_scene_presenter = std::make_unique<PlaterScenePresenter>(
        m_workbench,
        m_project_interactor,
        *m_device
    );
    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor.scene_interactor().add_listener<ISceneSelectionChangedListener>(this);
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);

    // Set our color styles before gizmos initialization
    // to use them during GiymoDialogs creation
    AbstractRenderLayout::set_our_style_colors();

    init_gizmos();
    init_scene();
    init_scene_layout();
    init_dialog_navigation();

    if (!m_thumbnail_image_generator->initialized()) {
        m_thumbnail_image_generator->init(m_workbench, *m_device, *m_scene_presenter);
    }
    m_scene_presenter->add_listener<Plater::IBedVisuallyChangedListener>(
        m_thumbnail_store_updater.get()
    );

    m_scene_presenter->force_bed_thumbnails_generation();

    m_scene_presenter->scene().set_lights(Slic3r::App::global_lighting());
}

void PlaterRenderModule::init_scene_layout()
{
    ASSERT(m_render_module_navigator);
    // >> This code is same for Plater/PreviewRenderModule
    m_top_bar = std::make_unique<TopBar>(&m_project_interactor, this, *m_thumbnail_store);

    m_object_list = Passthrough(std::make_unique<ObjectListWindow>(&m_project_interactor, true));

    m_cube_view   = Passthrough{std::make_unique<CubeView>()};
    m_sidebar_bed = Passthrough(
        std::make_unique<SidebarBed>(m_project_interactor, *m_render_module_navigator)
    );
    m_sidebar_print = Passthrough(
        std::make_unique<SidebarPrint>(m_project_interactor, *m_render_module_navigator)
    );
    m_sidebar_object = Passthrough(std::make_unique<SidebarObject>(m_project_interactor));
    m_pop_notification_list_view = Passthrough{
        std::make_unique<PopNotification::PopNotificationListView>(
            AppServices::instance().pop_notification_center()
        )
    };
    m_history = Passthrough(std::make_unique<History>());
    m_history->set_visible(false);

    m_sidebar_action_buttons = Passthrough{
        std::make_unique<SidebarPlaterActionButtons>(m_render_module_navigator)
    };
    m_sidebar_action_buttons->on_init(&m_project_interactor);

    m_layout.reset(new PlaterRenderLayout(
        m_top_bar.release(),
        m_object_list.release(),
        m_cube_view.release(),
        m_pop_notification_list_view.release(),
        m_sidebar_bed.release(),
        m_sidebar_print.release(),
        m_sidebar_object.release(),
        m_sidebar_action_buttons.release(),
        m_history.release()
    ));
    m_layout->init();

    // init toolbars
    m_layout->add_toolbar_item_panel(
        ToolbarID::Top,
        Render::Icon::ToolbarObjects,
        "Object List",
        "Ctrl + Alt + O",
        {},
        m_object_list.get()
    );

    // m_layout->add_toolbar_item_panel(
    //     ToolbarID::Bottom,
    //     Render::Icon::ToolbarHistory,
    //     "Actions History",
    //     "Shift + Alt + H",
    //     {},
    //     m_history.get()
    // );

    m_layout->add_toolbar_item(
        ToolbarID::Middle,
        Render::Icon::ToolbarAdd,
        "Add...",
        "Ctrl + I",
        {.action = [this]() {
        IDialogManager::FileCallback callback =
            [this](bool success, const std::vector<boost::filesystem::path>& file_paths) {
            if (success) {
                const auto& proj = m_workbench.project(m_project_interactor.selected_project_id());
                Domain::BedRef selected_bed = m_project_interactor.scene_interactor()
                                                  .bed_selection()
                                                  .last_selected_bed();
                const Domain::ConfigContainer* cc = proj.find_config_container(
                    selected_bed.config_container_id
                );
                const Domain::BedInstance& inst = cc->find_bed_instance(selected_bed.instance_id);
                int nozzle_dmrs_cnt             = cc->selected_preset().hw_config.tool_count;
                Biz::FileLoadingLogic::import_files_and_add_to_scene(
                    file_paths,
                    nozzle_dmrs_cnt,
                    m_project_interactor.scene_interactor(),
                    cc->bed().center()
                        + Biz::Algorithms::Point::to_2d(inst.transformation.get_offset()),
                    &App::AppServices::instance().dialog_manager()
                );

                m_scene_presenter->scene().log_nodes();
            }
        };

        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_file_dialog(
            FileDialogType::OpenMultiple,
            _u8L("Import File"),
            "",
            "",
            "STL (*.stl)|*.stl|3MF (*.3mf)|*.3mf",
            callback
        );
    }}
    );

    m_toolbar_add_volume = m_layout->add_toolbar_item(
        ToolbarID::Middle,
        Render::Icon::AddVolume,
        "Add Volume",
        "",
        {.action = [this]() {
        m_add_volumes_menu->open();
    }}
    );
    m_toolbar_add_volume->set_enabled(false);
    init_add_volume_menu();

    m_toolbar_delete = m_layout->add_toolbar_item(
        ToolbarID::Middle,
        Render::Icon::DeleteBtnIcon,
        "Delete selection",
        "",
        {.action = [this]()
         {
             std::optional<std::string>
                 last_solid_part_name = m_project_interactor.scene_interactor().delete_selected_elements();

             if (last_solid_part_name) {
                 // Show warning dialog
                 auto& dlg_manager = App::AppServices::instance().dialog_manager();
                 dlg_manager.show_warning_dialog(
                     fmt::vformat(
                         _u8L(
                             "Part {} could not be deleted from the object,\n"
                             "as removing the last solid part is not permitted."
                         ),
                         fmt::make_format_args(last_solid_part_name.value())
                     ) + "\n",
                     _u8L("Delete selection")
                 );
             }
         }}
    );
    m_toolbar_delete->set_enabled(false);

    m_toolbar_add_instance = m_layout->add_toolbar_item(
        ToolbarID::Middle,
        Render::Icon::ToolbarAddInstance,
        "Add instance",
        "+",
        {.action = [this]() {
        m_project_interactor.scene_interactor().add_instance(Domain::Vec2d(10., 5.));
        m_scene_presenter->scene().log_nodes();
    }}
    );
    m_toolbar_add_instance->set_enabled(false);

    m_toolbar_move = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarMove,
        "Move",
        "M",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::Translation);
    }},
        m_translation_gizmo
    );
    m_toolbar_move->set_enabled(false);
    m_toolbar_rotate = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarRotation,
        "Rotate",
        "R",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::Rotation);
    }},
        m_rotation_gizmo
    );
    m_toolbar_rotate->set_enabled(false);
    m_toolbar_arrange = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarArrange,
        "Arrange",
        "A",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::ArrangeGizmo);
    }},
        m_arrange_gizmo
    );
    m_toolbar_simplify = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarGraph,
        "Simplify",
        "B",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::Simplify);
    }},
        m_simplify_gizmo
    );
    m_toolbar_simplify->set_enabled(false);

    m_toolbar_paint_on_supports = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarPaintOnSupports,
        "Paint-on supports",
        "L",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::PaintOnSupportsGizmo);
    }},
        m_paint_on_supports_gizmo
    );

    m_toolbar_text = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarText,
        "Text",
        "T",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::TextGizmo);
    }},
        m_text_gizmo
    );

    m_toolbar_measure = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle,
        Render::Icon::ToolbarMeasure,
        "Measure",
        "U",
        {.action =
             [this]() {
        toggle_activate_tool(Scene::ToolType::MeasureGizmo);
    }},
        m_measure_gizmo
    );
}

void PlaterRenderModule::init_dialog_navigation()
{
    // Init sidebar dialogs
    m_dialog_navigation.insert_dialog(&m_sidebar_bed->logical_printer_settings_dialog());
    m_dialog_navigation.insert_dialog(
        &m_sidebar_bed->printer_add_dialog(),
        &m_sidebar_bed->logical_printer_settings_dialog()
    );
    m_dialog_navigation.insert_dialog(
        &m_sidebar_bed->logical_printer_settings_dialog().printer_advanced_settings_dialog(),
        &m_sidebar_bed->logical_printer_settings_dialog()
    );

    m_dialog_navigation.insert_dialog(&m_sidebar_bed->material_selection_dialog());
    m_dialog_navigation.insert_dialog(
        &m_sidebar_bed->material_selection_dialog().material_settings_dialog(),
        &m_sidebar_bed->material_selection_dialog()
    );

    m_dialog_navigation.insert_dialog(&m_sidebar_print->print_settings_dialog());

    // Init gizmos dialogs
    auto init_gizmo_dialog = [this](GizmoDialog* dialog) {
        dialog->gizmo_callbacks().close_requested = [this] {
            m_gizmo_manager->deactivate_current_tool();
            m_render_module_navigator->set_opened_dialog(nullptr);
        };
        m_dialog_navigation.insert_dialog(dialog);
        m_gizmo_dialogs.insert(dialog);
    };

    init_gizmo_dialog(m_arrange_gizmo->ui_dialog());
    init_gizmo_dialog(m_measure_gizmo->ui_dialog());
    init_gizmo_dialog(m_paint_on_supports_gizmo->ui_dialog());
    init_gizmo_dialog(m_simplify_gizmo->ui_dialog());
    init_gizmo_dialog(m_text_gizmo->ui_dialog());
}

void PlaterRenderModule::update_tool_selection(Scene::ToolType current_tool_type)
{
    m_toolbar_move->set_checked(current_tool_type == Scene::ToolType::Translation);
    m_toolbar_rotate->set_checked(current_tool_type == Scene::ToolType::Rotation);
    m_toolbar_simplify->set_checked(current_tool_type == Scene::ToolType::Simplify);
    m_toolbar_arrange->set_checked(current_tool_type == Scene::ToolType::ArrangeGizmo);
    m_toolbar_paint_on_supports->set_checked(
        current_tool_type == Scene::ToolType::PaintOnSupportsGizmo
    );
    m_toolbar_text->set_checked(current_tool_type == Scene::ToolType::TextGizmo);
    m_toolbar_measure->set_checked(current_tool_type == Scene::ToolType::MeasureGizmo);
}

void PlaterRenderModule::toggle_activate_tool(Scene::ToolType tool_type)
{
    m_gizmo_manager->toggle_activate_tool(tool_type, Domain::PrinterTechnology::FFF);

    switch (m_gizmo_manager->current_tool_type()) {
    case Scene::ToolType::Simplify:
        m_render_module_navigator->set_opened_dialog(m_simplify_gizmo->ui_dialog());
        break;
    case Scene::ToolType::ArrangeGizmo:
        m_render_module_navigator->set_opened_dialog(m_arrange_gizmo->ui_dialog());
        break;
    case Scene::ToolType::PaintOnSupportsGizmo:
        m_render_module_navigator->set_opened_dialog(m_paint_on_supports_gizmo->ui_dialog());
        break;
    case Scene::ToolType::TextGizmo:
        m_render_module_navigator->set_opened_dialog(m_text_gizmo->ui_dialog());
        break;
    case Scene::ToolType::MeasureGizmo:
        m_render_module_navigator->set_opened_dialog(m_measure_gizmo->ui_dialog());
        break;
    case Scene::ToolType::None:
        m_render_module_navigator->set_opened_dialog(nullptr);
    default:
        break;
    }
}

void PlaterRenderModule::init_scene()
{
    m_scene_presenter->scene().log_nodes();
}

void PlaterRenderModule::init_gizmos()
{
    // TODO: It shoud be OS dependent by maximal time betweem clic to be double click
    int min_drag_time_span = 500; // [in ms]
    // TODO: Load constant from OS
    int min_drag_offset = 500; // [in um]
    auto drag_detector = std::make_unique<Scene::MouseDragDetector>(min_drag_time_span, min_drag_offset);
    m_gizmo_manager = std::make_unique<Scene::GizmoManager>(
        *m_device,
        *m_scene_presenter,
        m_project_interactor,
        std::move(drag_detector)
    );
    m_gizmo_manager->add_listener<IGizmoActiveToolListener>(this);
    m_camera_gizmo = &m_gizmo_manager
                          ->add_base_gizmo<PlaterCameraGizmo>(m_workbench, *m_scene_presenter);
    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        m_camera_gizmo
    );
    BedSelectGizmo& bed_select_gizmo{m_gizmo_manager->add_base_gizmo<BedSelectGizmo>(
        m_project_interactor.scene_interactor(),
        *m_scene_presenter
    )};
    m_gizmo_manager->add_base_gizmo<QuickSelectGizmo>(
        m_project_interactor.scene_interactor(),
        *m_device,
        *m_scene_presenter,
        m_screen_info
    );
    m_gizmo_manager->add_base_gizmo<QuickDragGizmo>(
        m_project_interactor.scene_interactor(),
        *m_scene_presenter
    );
    m_translation_gizmo = &m_gizmo_manager->add_tool_gizmo<TranslationGizmo>(
        *m_device,
        m_gizmo_manager->data_factory(),
        *m_scene_presenter,
        m_project_interactor.scene_interactor()
    );
    m_rotation_gizmo = &m_gizmo_manager->add_tool_gizmo<RotationGizmo>(
        *m_device,
        m_gizmo_manager->data_factory(),
        *m_scene_presenter,
        m_project_interactor.scene_interactor()
    );
    m_arrange_gizmo = &m_gizmo_manager->add_tool_gizmo<ArrangeGizmo>(
        m_project_interactor.arrange_interactor(),
        *m_device,
        *m_scene_presenter,
        m_gizmo_manager->data_factory(),
        m_project_interactor,
        m_workbench
    );

    SimplifyGizmo::CloseFn close_fn = [mng = m_gizmo_manager.get()]() {
        mng->deactivate_current_tool();
    };
    m_simplify_gizmo = &m_gizmo_manager->add_tool_gizmo<SimplifyGizmo>(
        *m_device,
        *m_scene_presenter,
        m_project_interactor,
        close_fn
    );
    m_paint_on_supports_gizmo = &m_gizmo_manager->add_tool_gizmo<PaintOnSupportsGizmo>();
    m_text_gizmo              = &m_gizmo_manager->add_tool_gizmo<TextGizmo>();
    m_measure_gizmo           = &m_gizmo_manager->add_tool_gizmo<MeasureGizmo>(
        *m_device,
        m_project_interactor,
        *m_scene_presenter
    );
    m_project_interactor.scene_interactor().add_listener<Biz::Scene::ISceneSelectionChangedListener>(
        m_measure_gizmo
    );
}

void PlaterRenderModule::init_add_volume_menu()
{
    m_add_volumes_menu = std::make_unique<Yoga::Menu>(
        m_toolbar_add_volume,
        "add_volume_menu",
        Yoga::Position::Right
    );

    m_add_volumes_menu
        ->append_item(_u8L("Solid Part Volume"), nullptr, Render::Icon::SolidPartVolume)
        ->callbacks()
        .action = [this]() {
        add_volume(Domain::ModelVolumeType::MODEL_PART);
    };
    m_add_volumes_menu->append_item(_u8L("Negative Volume"), nullptr, Render::Icon::NegativeVolume)
        ->callbacks()
        .action = [this]() {
        add_volume(Domain::ModelVolumeType::NEGATIVE_VOLUME);
    };
    m_add_volumes_menu->append_item(_u8L("Modifier Volume"), nullptr, Render::Icon::ModifierVolume)
        ->callbacks()
        .action = [this]() {
        add_volume(Domain::ModelVolumeType::PARAMETER_MODIFIER);
    };
    m_add_volumes_menu->append_item(_u8L("Support Blocker"), nullptr, Render::Icon::SupportBlocker)
        ->callbacks()
        .action = [this]() {
        add_volume(Domain::ModelVolumeType::SUPPORT_BLOCKER);
    };
    m_add_volumes_menu
        ->append_item(_u8L("Support Modifier"), nullptr, Render::Icon::SupportModifier)
        ->callbacks()
        .action = [this]() {
        add_volume(Domain::ModelVolumeType::SUPPORT_ENFORCER);
    };
}

void PlaterRenderModule::add_volume(const Domain::ModelVolumeType& type)
{
    assert(m_add_volumes_menu->opened());
    m_add_volumes_menu->close();

    IDialogManager::FileCallback callback =
        [this, type](bool success, const std::vector<boost::filesystem::path>& file_paths) {
        if (success) {
            Biz::FileLoadingLogic::import_volumes_into_selected_object(
                file_paths,
                type,
                m_project_interactor.scene_interactor(),
                &AppServices::instance().dialog_manager()
            );

            m_scene_presenter->scene().log_nodes();
        }
    };

    auto& dlg_manager = AppServices::instance().dialog_manager();
    dlg_manager.show_file_dialog(
        FileDialogType::OpenMultiple,
        _u8L("Import File"),
        "",
        "",
        "STL (*.stl)|*.stl|3MF (*.3mf)|*.3mf",
        callback
    );
}

void PlaterRenderModule::active_tool_changed(Scene::IToolGizmo* active_tool)
{
    update_tool_selection(active_tool ? active_tool->type() : Scene::ToolType::None);
}

void PlaterRenderModule::set_navigator(Navigator* navigator)
{
    m_render_module_navigator = navigator;
}

void PlaterRenderModule::on_status_cache_status_code_changed(const Domain::SlicingId id)
{
    // request redraw
    request_render();
}

void PlaterRenderModule::set_sidebars_visible(bool visible)
{
    m_layout->set_sidebars_visible(visible);

    // request redraw
    request_render();
}

Platform::CameraSynchData PlaterRenderModule::camera_synch_data()
{
    Platform::CameraSynchData ret;
    m_scene_presenter->scene().camera().update_synch_data(ret);
    m_scene_presenter->scene().camera_trackball().update_synch_data(ret);
    return ret;
}

void PlaterRenderModule::set_camera_synch_data(const Platform::CameraSynchData& data)
{
    if (m_scene_presenter == nullptr)
        return;

    synchronize_camera(
        data,
        m_scene_presenter->scene().camera(),
        m_scene_presenter->scene().camera_trackball()
    );
}

void PlaterRenderModule::render_scene(Render::CommandBuffer& cmd_buffer)
{
    Render::ScopedDebugGroup event_imgui_render("Plater Render", cmd_buffer);
    m_device->load_state();

    cmd_buffer.set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer.set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer.clear_buffers(true, true);

    m_scene_presenter->render_scene(cmd_buffer);

    m_gizmo_manager->render_scene(cmd_buffer);

    cmd_buffer.submit();
}

#if ENABLED_DEBUG_BEDS
static void render_imgui_debug_bed(
    Biz::ProjectInteractor& project_interactor,
    PlaterScenePresenter& scene_presenter,
    Render::Device& device
)
{

    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Bed test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto& proj                       = project_interactor.selected_project();
        auto& scene_interactor           = project_interactor.scene_interactor();
        const Domain::BedRef& active_tag = scene_interactor.bed_selection().last_selected_bed();

        size_t total_instances_count                    = 0;
        const Domain::Project::ConfigContainerList& ccs = proj.config_containers();
        for (auto& cc : ccs) {
            total_instances_count += cc->bed_instances().size();
        }

        Domain::BedRef remove_tag{Domain::INVALID_ID, Domain::INVALID_ID};

        if (ImGui::BeginTable("Beds", (total_instances_count > 1) ? 5 : 4, ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Container ID");
            ImGui::TableSetupColumn("Instance ID");
            ImGui::TableSetupColumn("Model Insts");
            ImGui::TableSetupColumn("Print Volume");
            ImGui::TableHeadersRow();

            Scene::visit(
                scene_presenter.scene().root(),
                [&](Scene::Node& n)
                {
                    Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
                    if (tag != nullptr) {
                        Domain::ConfigContainer* cc = proj.find_config_container(tag->config_container_id);
                        DEBUG_ASSERT(cc != nullptr);
                        Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                        if (tag->type == Scene::BedElementType::Undefined) {
                            bool active = active_tag.config_container_id == tag->config_container_id
                                && active_tag.instance_id == tag->instance_id;

                            ImGui::TableNextRow();
                            if (active)
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableHeaderBg));

                            ImGui::TableSetColumnIndex(0);
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("%zu", tag->config_container_id);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%zu", tag->instance_id);

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%zu", inst.model_instances.size());

                            ImGui::TableSetColumnIndex(3);
                            bool print_volume = inst.print_volume_enabled;
                            if (ImGui::Checkbox(
                                    fmt::format("##print_volume{}/{}", tag->config_container_id, tag->instance_id)
                                        .c_str(),
                                    &print_volume
                                ))
                            {
                                inst.print_volume_enabled = print_volume;
                                scene_presenter.update_beds();
                            }

                            if (total_instances_count > 1) {
                                ImGui::TableSetColumnIndex(4);
                                if (ImGui::Button(
                                        fmt::format("Remove##{}/{}", tag->config_container_id, tag->instance_id)
                                            .c_str()
                                    ))
                                    remove_tag = {tag->config_container_id, tag->instance_id};
                            }
                        }
                    }
                }
            );

            ImGui::EndTable();
        }

        if (remove_tag.config_container_id != Domain::INVALID_ID) {
            const Domain::BedRef& active = scene_interactor.bed_selection().last_selected_bed();
            scene_interactor.remove_bed_instance(remove_tag);
            if (active == remove_tag) {
                auto& cc{project_interactor.selected_config_container()};
                auto& inst{cc.bed_instances().front()};
                scene_interactor.bed_selection().select_one(Domain::BedRef{cc.id().id, inst->id().id});
            }
            --total_instances_count;
        }

        if (total_instances_count < 9) {
            if (ImGui::Button("Add instance"))
                scene_interactor.add_bed_instance(
                    project_interactor.selected_config_container().id().id
                );
        }
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_BEDS

void PlaterRenderModule::render_imgui(Render::CommandBuffer& cmd_buffer)
{
    if (!m_scene_presenter->project_ready())
        return;

    m_thumbnail_image_generator->handle_enqueued_requests();
    m_thumbnail_store_updater->update(*m_device, [this](const BedThumbnailTextures& textures) {
        m_object_list->set_bed_instance_icons(textures);
    });


    m_cube_view->set_camera_data(m_scene_presenter->scene().camera(), m_scene_presenter->scene().camera_trackball());

    m_layout->render(Vec2f(m_screen_info.logical_width(), m_screen_info.logical_height()));

    if (m_cube_view->require_render())
        request_render();

    m_scene_presenter->render_imgui(m_screen_info);
    m_gizmo_manager->render_imgui();
#if ENABLED_DEBUG_OUTLINE
    if (ImGui::Begin("Outline", nullptr))
        imgui_scenegraph_node_info(m_scene_presenter->scene().root());
    ImGui::End();
#endif // ENABLED_DEBUG_OUTLINE

#if ENABLED_DEBUG_BEDS
    // ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->GetCenter().x, 50.f), ImGuiCond_Always);
    render_imgui_debug_bed(m_project_interactor, *m_scene_presenter, *m_device);
#endif // ENABLED_DEBUG_BEDS

#if ENABLED_DEBUG_CAMERA
    render_imgui_debug_camera(
        m_scene_presenter->scene().camera(),
        m_scene_presenter->scene().camera_trackball()
    );
#endif // ENABLED_DEBUG_CAMERA
    Scene::render_imgui_graphics_settings_debug_window(m_project_interactor.selected_project(), *m_device, *m_scene_presenter, *m_imgui_render);
}

void PlaterRenderModule::render_object_hud(
    const Scene::Node& n,
    const Eigen::AlignedBox<float, 2>& screen_bounding_box
)
{
    std::string node_name = "##node_hud_" + std::to_string(reinterpret_cast<size_t>(&n));

    ImGui::SetNextWindowPos({screen_bounding_box.max().x(), screen_bounding_box.min().y()});
    if (ImGui::Begin(
            node_name.c_str(),
            nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground
        ))
    {
        if (ImGui::SmallButton("Foc"))
            m_scene_presenter->scene().camera_trackball().set_target(Vec3d::Zero());
    }
    ImGui::End();
}

void PlaterRenderModule::on_scene_mouse_event(const Platform::MouseEvent& e)
{
    m_gizmo_manager->on_scene_mouse_event(e, m_screen_info);
    m_scene_presenter->update_sinking_contours_visibility(e, m_screen_info);
}

void PlaterRenderModule::on_scene_keyboard_event(const Platform::KeyboardEvent& e)
{
    if (!m_gizmo_manager->on_scene_keyboard_event(e))
        Platform::AbstractRenderModule::on_scene_keyboard_event(e);
}

void PlaterRenderModule::on_activated()
{
    if (m_scene_presenter != nullptr)
        m_scene_presenter->scene().set_lights(App::global_lighting());

    if (m_object_list.get() != nullptr)
        // object list icons may have been updated while the preview was active
        m_object_list->set_bed_instance_icons(m_thumbnail_store->projects.selected().thumbnails);
}

void PlaterRenderModule::on_deactivated()
{
    App::set_global_lighting(m_scene_presenter->scene().lights());
}

void PlaterRenderModule::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    const bool empty_selection = selection.empty();
    m_toolbar_move->set_enabled(!empty_selection);
    m_toolbar_rotate->set_enabled(!empty_selection);
    m_toolbar_simplify->set_enabled(!empty_selection);
    m_toolbar_delete->set_enabled(!empty_selection);

    m_text_gizmo->update_layout(
        !empty_selection && selection.mode == Slic3r::Biz::Scene::SelectionMode::Volume
    );

    bool can_add_instance = !empty_selection;
    if (can_add_instance) {
        const size_t obj_id = selection.elements[0].object_id;
        for (const Domain::ElementRef& el : selection.elements) {
            if (el.object_id != obj_id) {
                // We can’t add instances for multiple objects simultaneously.
                can_add_instance = false;
                break;
            }
        }
    }
    m_toolbar_add_instance->set_enabled(can_add_instance);

    m_toolbar_add_volume->set_enabled(can_add_instance);
    if (!can_add_instance && m_add_volumes_menu->opened()) {
        m_add_volumes_menu->close();
    }

    m_sidebar_bed->set_visible(empty_selection);
    m_sidebar_print->set_visible(empty_selection);
    m_sidebar_object->set_visible(!empty_selection);
}

void PlaterRenderModule::on_screen_resized()
{
    // m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    auto viewport = Render::Rect::from(0, 0, m_screen_info);
    m_scene_presenter->screen_resized(viewport);
}

void PlaterRenderModule::on_set_imgui_render() {}

void PlaterRenderModule::on_selected_project_changed(size_t index)
{
    m_object_list->update_del_button();
}

} // namespace Slic3r::App::Plater
