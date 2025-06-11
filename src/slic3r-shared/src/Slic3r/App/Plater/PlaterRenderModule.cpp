#include "Slic3r/App/Plater/PlaterRenderModule.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Scene/LightingHelper.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
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
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/IRenderModuleChangedListener.hpp"

#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/SidebarActionButtons.hpp"
#include "Slic3r/App/LightSetting.hpp"
#include "Slic3r/App/BedStore.hpp"
#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"
#include "Slic3r/App/Yoga/ToolbarButton.hpp"

#include <imgui/imgui.h>
#include <Eigen/SVD>

#define ENABLED_DEBUG_OUTLINE 1
#define ENABLED_DEBUG_IMGUI_FONT 0
#define ENABLED_DEBUG_IMGUI_ICONS 0
#define ENABLED_DEBUG_BEDS 1
#define ENABLED_DEBUG_CAMERA 0

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

namespace TriMesh = Biz::Algorithms::TriangleMesh;

PlaterRenderModule::PlaterRenderModule(
    const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor
)
    : m_workbench(workbench), m_project_interactor(project_interactor)
{}

PlaterRenderModule::~PlaterRenderModule()
{
    if (m_gizmo_manager)
        m_gizmo_manager->remove_listener<IGizmoActiveToolListener>(this);
}

void PlaterRenderModule::on_init(Render::Device& device, Render::ImguiRender& imgui_render)
{
    AbstractRenderModule::on_init(device, imgui_render);
    Yoga::Item::set_imgui_render(&imgui_render
    ); // Todo: move this somewhere where it is invoked once
    m_scene_presenter =
        std::make_unique<PlaterScenePresenter>(m_workbench, m_project_interactor, *m_device);
    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    init_gizmos();
    init_scene();
    init_scene_layout();

    m_scene_presenter->scene().set_lights(Slic3r::App::global_lighting());
}

void PlaterRenderModule::add_type_changed_listener(IRenderModuleChangedListener* l)
{
    m_render_module_changed_listeners.insert(l);

}

void PlaterRenderModule::remove_type_changed_listener(IRenderModuleChangedListener* l)
{
    m_render_module_changed_listeners.erase(l);
}

void PlaterRenderModule::init_scene_layout()
{
    AbstractRenderLayout::set_our_style_colors();

    // >> This code is same for Plater/PreviewRenderModule
    m_top_bar = std::make_unique<TopBar>(&m_project_interactor, this);

    m_object_list = Passthrough(std::make_unique<ObjectListWindow>(&m_project_interactor, true));

    m_cube_view = Passthrough{std::make_unique<CubeView>()};
    m_sidebar_bed = Passthrough(std::make_unique<SidebarBed>());
    m_sidebar_print = Passthrough(std::make_unique<SidebarPrint>());
    m_history = Passthrough(std::make_unique<History>());
    m_history->set_visible(false);

    m_sidebar_action_buttons = Passthrough{std::make_unique<SidebarPlaterActionButtons>()};
    m_sidebar_action_buttons->on_init(&m_project_interactor);
    for (IRenderModuleChangedListener* listener : std::as_const(m_render_module_changed_listeners)) {
        m_sidebar_action_buttons->add_listener<IRenderModuleChangedListener>(listener);
    }

    m_layout.reset(new PlaterRenderLayout(
        m_top_bar.release(), m_object_list.release(), m_cube_view.release(), m_sidebar_bed.release(),
        m_sidebar_print.release(), m_sidebar_action_buttons.release(), m_history.release()
    ));
    m_layout->init();

    // init toolbars
    m_layout->add_toolbar_item_panel(
        ToolbarID::Top, Render::Icon::ToolbarObjects, "Object List", "Ctrl + Alt + O", {},
        m_object_list.get()
    );

    m_layout->add_toolbar_item_panel(
        ToolbarID::Bottom, Render::Icon::ToolbarHistory, "Actions History", "Shift + Alt + H", {},
        m_history.get()
    );

    m_layout->add_toolbar_item(
        ToolbarID::Middle, Render::Icon::ToolbarAdd, "Add...", "Ctrl + I",
        {.action =
             [this]() {
                 auto& scene_interactor = m_project_interactor.scene_interactor();
                 const auto& bed =
                     m_project_interactor.selected_project().config_containers().front()->bed();

                 scene_interactor.new_object_from_mesh(TriMesh::make_cube(10, 15, 20));

                 Transform3d xform = Transform3d::Identity();
                 xform.translate(Vec3d{bed.center().x(), bed.center().y(), 0});
                 scene_interactor.transform_selection(xform.matrix());

                 m_scene_presenter->scene().log_nodes();
             }}
    );
    //    m_layout.add_toolbar_item(ToolbarID::Middle, ImGui::ToolbarArrange, "Arrange", "A", { []() {} });
    m_toolbar_move = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle, Render::Icon::ToolbarMove, "Move", "M",
        {.action = [this]() { toggle_activate_tool(Scene::ToolType::Translation); }},
        m_translation_gizmo
    );
    m_toolbar_rotate = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle, Render::Icon::ToolbarRotation, "Rotate", "R",
        {.action = [this]() { toggle_activate_tool(Scene::ToolType::Rotation); }}, m_rotation_gizmo
    );
    m_toolbar_simplify = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle, Render::Icon::ToolbarGraph, "Simplify", "B", 
        {.action = [this]() { toggle_activate_tool(Scene::ToolType::Simplify); }},
        m_simplify_gizmo
    );
    m_toolbar_paint_on_supports = m_layout->add_toolbar_item_gizmo(
        ToolbarID::Middle, Render::Icon::ToolbarPaintOnSupports, "Paint-on supports", "L",
        {.action = [this]() { toggle_activate_tool(Scene::ToolType::PaintOnSupportsGizmo); }},
        m_paint_on_supports_gizmo
    );
}

void PlaterRenderModule::update_toolbar_tool_selection(Scene::ToolType current_tool_type)
{
    m_toolbar_move->set_checked(current_tool_type == Scene::ToolType::Translation);
    m_toolbar_rotate->set_checked(current_tool_type == Scene::ToolType::Rotation);
    m_toolbar_paint_on_supports->set_checked(
        current_tool_type == Scene::ToolType::PaintOnSupportsGizmo
    );
}

void PlaterRenderModule::toggle_activate_tool(Scene::ToolType tool_type)
{
    m_gizmo_manager->toggle_activate_tool(tool_type, ptFFF);

    Scene::ToolType current_tool_type = m_gizmo_manager->current_tool_type();
    update_toolbar_tool_selection(current_tool_type);
}

#if 0
void override_config(ModelConfigObject& config)
{
    config.set_key_value("fill_pattern", new ConfigOptionEnum<InfillPattern>(ipHoneycomb));
    config.set_key_value("extruder", new ConfigOptionInt(3));
}
#endif

void PlaterRenderModule::init_scene()
{
    m_scene_presenter->scene().log_nodes();
    m_scene_presenter->update_objects_shadows_data();
}

void PlaterRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<Scene::GizmoManager>(*m_device, *m_scene_presenter, m_project_interactor);
    m_gizmo_manager->add_listener<IGizmoActiveToolListener>(this);
    PlaterCameraGizmo* camera_gizmo =
        &m_gizmo_manager->add_base_gizmo<PlaterCameraGizmo>(m_workbench, *m_scene_presenter);
    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstanceChangedListener>(
        camera_gizmo
    );
    m_gizmo_manager->add_base_gizmo<QuickSelectGizmo>(
        m_project_interactor.scene_interactor(), *m_device, *m_scene_presenter, m_screen_info
    );
    m_gizmo_manager
        ->add_base_gizmo<BedSelectGizmo>(m_project_interactor.scene_interactor(), *m_scene_presenter);
    m_gizmo_manager
        ->add_base_gizmo<QuickDragGizmo>(m_project_interactor.scene_interactor(), *m_scene_presenter);
    m_translation_gizmo = &m_gizmo_manager->add_tool_gizmo<TranslationGizmo>(
        *m_device, m_gizmo_manager->data_factory(), *m_scene_presenter,
        m_project_interactor.scene_interactor()
    );
    m_rotation_gizmo = &m_gizmo_manager->add_tool_gizmo<RotationGizmo>(
        *m_device, m_gizmo_manager->data_factory(), *m_scene_presenter,
        m_project_interactor.scene_interactor()
    );
    SimplifyGizmo::CloseFn close_fn = [mng = m_gizmo_manager.get()]() { mng->deactivate_current_tool(); };
    m_simplify_gizmo = &m_gizmo_manager->add_tool_gizmo<SimplifyGizmo>(
        *m_device, *m_scene_presenter, m_project_interactor, close_fn);
    m_paint_on_supports_gizmo = &m_gizmo_manager->add_tool_gizmo<PaintOnSupportsGizmo>();
}

void PlaterRenderModule::active_tool_changed(Scene::IToolGizmo* active_tool)
{
    update_toolbar_tool_selection(active_tool ? active_tool->type() : Scene::ToolType::None);
}


void PlaterRenderModule::on_status_cache_changed(const Biz::Slicing::SlicingId id)
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

void PlaterRenderModule::synchronize_topbar()
{
    m_layout->synchronize_topbar();
    // request redraw
    request_render();
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

class ImguiVecRender
{
public:
    void operator()(const char* label, const Vec2f& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }

    void operator()(const char* label, const Vec2d& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }
    //
    // void operator()(const char* label, const Vec3f& v)
    // {
    //     fill_data<3>(v);
    //     ImGui::InputFloat3(label, m_data);
    // }
    //
    void operator()(const char* label, const Vec3d& v)
    {
        fill_data<3>(v);
        ImGui::InputFloat3(label, m_data);
    }

    void operator()(const char* label, const Vec4f& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

    void operator()(const char* label, const Vec4d& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

private:
    template<size_t N, typename VecT>
    void fill_data(const VecT& data)
    {
        for (size_t i = 0; i < N; i++)
            m_data[i] = static_cast<float>(data[i]);
    }

private:
    float m_data[4];
};

void imgui_scenegraph_node_info(const Scene::Node& node)
{
    ImGuiTreeNodeFlags node_flags = 0; // ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children().empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    const std::string& name = node.debug_name();
    if (ImGui::TreeNodeEx(
            &node, node_flags, "%s %s%s%s%s", name.empty() ? "Node" : name.c_str(),
            node.has_render_component() ? "(R)" : "", node.has_material_override() ? "(M)" : "",
            node.has_imgui_render_component() ? "(I)" : "", node.has_raycast_component() ? "(C)" : ""
        )) {

        static const Scene::Node* opened_node = nullptr;

        ImGui::SameLine();
        if (ImGui::SmallButton("info")) {
            opened_node = (opened_node == &node) ? nullptr : &node;
        }

        if (opened_node == &node) {
            auto transform{node.world_transform()};

            ImguiVecRender vec_render;
            for (size_t i = 0; i < 4; i++) {
                ImGui::PushID(i);
                vec_render("##", Vec4d{transform.row(i)});
                ImGui::PopID();
            }
        }

        for (const auto& ch : node.children()) {
            imgui_scenegraph_node_info(*ch);
        }
        ImGui::TreePop();
    }
}

#if ENABLED_DEBUG_IMGUI_FONT
static void render_imgui_debug_input_font()
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Fonts test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Fonts", 2, ImGuiTableFlags_Borders)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Czech");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("oddělitelné");
            ImGui::Text("žádné otevřené kotvy");
            ImGui::Text("Přerušit");
            ImGui::Text("Přesné");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Russian");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Неизвестно");
            ImGui::Text("Внешний периметр");
            ImGui::Text("Нависающие периметры");
            ImGui::Text("Внутреннее заполнение");

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_IMGUI_FONT

#if ENABLED_DEBUG_IMGUI_ICONS
static void render_imgui_debug_icons()
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("ImGui icons test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Icons", 2, ImGuiTableFlags_Borders)) {
            float font_scale = ImGui::GetTextLineHeight() / 15.0f;
            int icon_sz = lround(16 * font_scale);

            int px = icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<wchar_t, std::string>> ICONS = {
                {ImGui::PrintIconMarker, "cog"},
                {ImGui::PrinterIconMarker, "printer"},
                {ImGui::PrinterSlaIconMarker, "sla_printer"},
                {ImGui::FilamentIconMarker, "spool"},
                {ImGui::MaterialIconMarker, "resin"},
                {ImGui::MinimalizeButton, "notification_minimalize"},
                {ImGui::MinimalizeHoverButton, "notification_minimalize_hover"},
                {ImGui::RightArrowButton, "notification_right"},
                {ImGui::RightArrowHoverButton, "notification_right_hover"},
                {ImGui::PreferencesButton, "notification_preferences"},
                {ImGui::PreferencesHoverButton, "notification_preferences_hover"},
                {ImGui::SliderFloatEditBtnIcon, "edit_button"},
                {ImGui::SliderFloatEditBtnPressedIcon, "edit_button_pressed"},
                {ImGui::ClipboardBtnIcon, "copy_menu"},
                {ImGui::ExpandBtn, "expand_btn"},
                {ImGui::CollapseBtn, "collapse_btn"},
                {ImGui::RevertButton, "undo"},
                {ImGui::WarningMarkerSmall, "notification_warning"},
                {ImGui::InfoMarkerSmall, "notification_info"},
                {ImGui::PlugMarker, "plug"},
                {ImGui::DowelMarker, "dowel"},
                {ImGui::SnapMarker, "snap"},
                {ImGui::HorizontalHide, "horizontal_hide"},
                {ImGui::HorizontalShow, "horizontal_show"},
                {ImGui::PrintIdle, "print_idle"},
                {ImGui::PrintRunning, "print_running"},
                {ImGui::PrintFinished, "print_finished"},
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
                    Imgui::icon_button(
                        ICONS[i].first, ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f
                    );
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = int(1.25f * icon_sz);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons medium %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<wchar_t, std::string>> ICONS_MEDIUM = {
                {ImGui::Lock, "lock_closed"},
                {ImGui::LockHovered, "lock_closed_f"},
                {ImGui::Unlock, "lock_open"},
                {ImGui::UnlockHovered, "lock_open_f"},
                {ImGui::DSRevert, "undo_r"},
                {ImGui::DSRevertHovered, "undo_f"},
                {ImGui::DSRevertDisabled, "undo_disabled"},
                {ImGui::DSSettings, "cog"},
                {ImGui::DSSettingsHovered, "cog_f"},
                {ImGui::ErrorTick, "error_tick"},
                {ImGui::ErrorTickHovered, "error_tick_f"},
                {ImGui::PausePrint, "pause_print"},
                {ImGui::PausePrintHovered, "pause_print_f"},
                {ImGui::EditGCode, "edit_gcode"},
                {ImGui::EditGCodeHovered, "edit_gcode_f"},
                {ImGui::RemoveTick, "colorchange_del"},
                {ImGui::RemoveTickHovered, "colorchange_del_f"},
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_medium", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_MEDIUM.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
                    Imgui::icon_button(
                        ICONS_MEDIUM[i].first, ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f
                    );
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_MEDIUM[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = 2 * icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons large %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<wchar_t, std::string>> ICONS_LARGE = {
                {ImGui::LegendTravel, "legend_travel"},
                {ImGui::LegendWipe, "legend_wipe"},
                {ImGui::LegendRetract, "legend_retract"},
                {ImGui::LegendDeretract, "legend_deretract"},
                {ImGui::LegendSeams, "legend_seams"},
                {ImGui::LegendToolChanges, "legend_toolchanges"},
                {ImGui::LegendColorChanges, "legend_colorchanges"},
                {ImGui::LegendPausePrints, "legend_pauseprints"},
                {ImGui::LegendCustomGCodes, "legend_customgcodes"},
                {ImGui::LegendCOG, "legend_cog"},
                {ImGui::LegendShells, "legend_shells"},
                {ImGui::LegendToolMarker, "legend_toolmarker"},
                {ImGui::CloseNotifButton, "notification_close"},
                {ImGui::CloseNotifHoverButton, "notification_close_hover"},
                {ImGui::EjectButton, "notification_eject_sd"},
                {ImGui::EjectHoverButton, "notification_eject_sd_hover"},
                {ImGui::WarningMarker, "notification_warning"},
                {ImGui::ErrorMarker, "notification_error"},
                {ImGui::CancelButton, "notification_cancel"},
                {ImGui::CancelHoverButton, "notification_cancel_hover"},
                //                { ImGui::SinkingObjectMarker,     "move" }, {
                //                ImGui::CustomSupportsMarker,    "fdm_supports" }, {
                //                ImGui::CustomSeamMarker,        "seam" }, {
                //                ImGui::MmuSegmentationMarker,   "mmu_segmentation" }, {
                //                ImGui::VarLayerHeightMarker,    "layers" },
                {ImGui::DocumentationButton, "notification_documentation"},
                {ImGui::DocumentationHoverButton, "notification_documentation_hover"},
                {ImGui::InfoMarker, "notification_info"},
                {ImGui::PlayButton, "notification_play"},
                {ImGui::PlayHoverButton, "notification_play_hover"},
                {ImGui::PauseButton, "notification_pause"},
                {ImGui::PauseHoverButton, "notification_pause_hover"},
                {ImGui::OpenButton, "notification_open"},
                {ImGui::OpenHoverButton, "notification_open_hover"},
                {ImGui::SlaViewOriginal, "sla_view_original"},
                {ImGui::SlaViewProcessed, "sla_view_processed"},
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_large", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_LARGE.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
                    Imgui::icon_button(
                        ICONS_LARGE[i].first, ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f
                    );
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_LARGE[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = 4 * icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons extra large %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<wchar_t, std::string>> ICONS_EXTRA_LARGE = {
                {ImGui::ClippyMarker, "notification_clippy"},
                {ImGui::SliceAllBtnIcon, "slice_all"},
                {ImGui::WarningMarkerDisabled, "notification_warning_grey"},
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_extra_large", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_EXTRA_LARGE.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.0f, 0.0f, 0.0f, 0.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
                    Imgui::icon_button(
                        ICONS_EXTRA_LARGE[i].first,
                        ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f
                    );
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_EXTRA_LARGE[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_IMGUI_ICONS

#if ENABLED_DEBUG_BEDS
static void render_imgui_debug_bed(Biz::ProjectInteractor& project_interactor, PlaterScenePresenter& scene_presenter, Render::Device& device)
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Bed test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        auto& proj = project_interactor.selected_project();
        auto& scene_interactor = project_interactor.scene_interactor();
        const Domain::BedRef& active_tag = scene_interactor.selected_bed_instance();

        size_t total_instances_count = 0;
        const Domain::Project::ConfigContainerList& ccs = proj.config_containers();
        for (auto& cc : ccs) {
            total_instances_count += cc->bed_instances().size();
        }

        Domain::BedRef remove_tag{Domain::INVALID_ID, Domain::INVALID_ID};

        if (ImGui::BeginTable("Beds", (total_instances_count > 1) ? 6 : 5, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Container ID");
            ImGui::TableSetupColumn("Instance ID");
            ImGui::TableSetupColumn("Model Insts");
            ImGui::TableSetupColumn("Contour");
            ImGui::TableSetupColumn("Print Volume");
            ImGui::TableHeadersRow();

            Scene::visit(scene_presenter.scene().root(), [&](Scene::Node& n) {
                Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
                if (tag != nullptr) {
                    Domain::ConfigContainer* cc = proj.find_config_container(tag->config_container_id);
                    DEBUG_ASSERT(cc != nullptr);
                    Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                    if (tag->type == Scene::BedElementType::Undefined) {

                        bool active = active_tag.config_container_id == tag->config_container_id &&
                            active_tag.instance_id == tag->instance_id;

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
                        bool contour = inst.contour_enabled;
                        if (ImGui::Checkbox(fmt::format("##contour{}/{}", tag->config_container_id, tag->instance_id).c_str(),
                                &contour)) {
                            inst.contour_enabled = contour;
                            scene_presenter.update_beds();
                        }

                        ImGui::TableSetColumnIndex(4);
                        bool print_volume = inst.print_volume_enabled;
                        if (ImGui::Checkbox(fmt::format("##print_volume{}/{}", tag->config_container_id, tag->instance_id).c_str(),
                                &print_volume)) {
                            inst.print_volume_enabled = print_volume;
                            scene_presenter.update_beds();
                        }

                        if (total_instances_count > 1) {
                            ImGui::TableSetColumnIndex(5);
                            if (ImGui::Button(fmt::format("Remove##{}/{}", tag->config_container_id, tag->instance_id).c_str()))
                                remove_tag = {tag->config_container_id, tag->instance_id};
                        }
                    }
                }
            });

            ImGui::EndTable();
        }

        if (remove_tag.config_container_id != Domain::INVALID_ID) {
            const Domain::BedRef& active = scene_interactor.selected_bed_instance();
            scene_interactor.remove_bed_instance(remove_tag);
            if (active == remove_tag)
                scene_interactor.select_first_bed_instance();
            --total_instances_count;
        }

        if (total_instances_count < 9) {
            if (ImGui::Button("Add instance"))
                scene_interactor.add_bed_instance(project_interactor.selected_config_container().id().id);
        }

        ImGui::Separator();

        size_t texture_size = Scene::BedRenderHelper::texture_size();
        if (texture_size > 0) {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture size");
            ImGui::SameLine();

            std::vector<size_t> sizes;
            for (size_t i = 512; i <= Render::Context::instance().max_texture_size(); i *= 2){
                sizes.push_back(i);
            }
            
            std::vector<std::string> sizes_str;
            std::transform(sizes.begin(), sizes.end(), std::back_inserter(sizes_str), [](size_t size) {
                return std::to_string(size) + "x" + std::to_string(size);
            });

            auto it = std::find(sizes.begin(), sizes.end(), texture_size);
            DEBUG_ASSERT(it != sizes.end());
            int sel_size = int(std::distance(sizes.begin(), it));

            const char* preview_value = sizes_str[sel_size].c_str();

            if (ImGui::BeginCombo("##texture_sizes", preview_value)) {
                for (int i = 0; i < int(sizes_str.size()); i++) {
                    bool is_selected = (sel_size == i);
                    if (ImGui::Selectable(sizes_str[i].c_str(), is_selected))
                        sel_size = i;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // force bed textures reload with the new size
            if (sizes[sel_size] != texture_size) {
                Scene::BedRenderHelper::set_texture_size(sizes[sel_size]);
                Scene::visit(scene_presenter.scene().root(), [&](Scene::Node& n) {
                    Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
                    if (tag != nullptr) {
                        if (tag->type == Scene::BedElementType::PlateTextured) {
                            Domain::ConfigContainer* cc = proj.find_config_container(tag->config_container_id);
                            Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                            n.render_component()->replace_material(Scene::BedMaterials::plate_textured_material(device, inst.bed));
                            if (n.has_material_override())
                                n.set_material_override(Scene::BedMaterials::plate_textured_override_material(n.render_component()->material()));
                        }
                    }
                });
            }
        }
    }
    ImGui::End();
}
#endif //ENABLED_DEBUG_BEDS

#if ENABLED_DEBUG_CAMERA
static void render_imgui_debug_camera(const Scene::Camera& camera, const Scene::CameraTrackballController& trackball)
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Camera debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Camera", 2, ImGuiTableFlags_Borders)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(camera.position()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Target");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(trackball.target()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Distance to target");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", trackball.distance_to_target());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Pivot");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(trackball.pivot()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Azimuth");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", Geometry::rad2deg(trackball.azimuth()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Zenith");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", Geometry::rad2deg(trackball.zenith()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Forward");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(camera.forward()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Right");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(camera.right()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Up");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", to_string(camera.up()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Zoom");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", camera.zoom());

            auto& proj = camera.cam_projection();
            if (proj.type() == Scene::CameraProjectionType::Perspective) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("FOVy");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", dynamic_cast<const Scene::PerspectiveCameraProjection&>(proj).fovy());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("FOVy/Zoom");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", dynamic_cast<const Scene::PerspectiveCameraProjection&>(proj).fovy() / camera.zoom());
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_CAMERA

void PlaterRenderModule::render_imgui(Render::CommandBuffer & cmd_buffer)
{
    if (!m_scene_presenter->project_ready())
        return;

    m_layout->render(Vec2f(m_screen_info.logical_width(), m_screen_info.logical_height()));

    m_scene_presenter->render_imgui(m_screen_info);
    m_gizmo_manager->render_imgui();
#if ENABLED_DEBUG_OUTLINE
    if (ImGui::Begin("Outline", nullptr)) {
        imgui_scenegraph_node_info(m_scene_presenter->scene().root());
    }
    ImGui::End();
#endif // ENABLED_DEBUG_OUTLINE
#if ENABLED_DEBUG_IMGUI_FONT
    render_imgui_debug_input_font();
#endif // ENABLED_DEBUG_IMGUI_FONT
#if ENABLED_DEBUG_IMGUI_ICONS
    render_imgui_debug_icons();
#endif // ENABLED_DEBUG_IMGUI_ICONS

#if ENABLED_DEBUG_BEDS
    //ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->GetCenter().x, 50.f), ImGuiCond_Always);
    render_imgui_debug_bed(m_project_interactor, *m_scene_presenter, *m_device);
#endif // ENABLED_DEBUG_BEDS

#if ENABLED_DEBUG_CAMERA
    render_imgui_debug_camera(m_scene_presenter->scene().camera(), m_scene_presenter->scene().camera_trackball());
#endif // ENABLED_DEBUG_CAMERA
#if ENABLED_SCENE_SHADING_CUSTOMIZATION
    render_imgui_scene_shading_customization(*m_scene_presenter, [this]() { m_scene_presenter->update_beds_shadows_data(); });
#endif // ENABLED_SCENE_SHADING_CUSTOMIZATION
#if ENABLED_LIGHTS_CUSTOMIZATION
    render_imgui_lights_customization(*m_scene_presenter);
#endif // ENABLED_LIGHTS_CUSTOMIZATION
}

void PlaterRenderModule::render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box)
{
    std::string node_name = "##node_hud_" + std::to_string(reinterpret_cast<size_t>(&n));

    ImGui::SetNextWindowPos({
        screen_bounding_box.max().x(),
        screen_bounding_box.min().y()
    });
    if (ImGui::Begin(node_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground)) {
        if (ImGui::SmallButton("Foc"))
            m_scene_presenter->scene().camera_trackball().set_target(Vec3d::Zero());
    }
    ImGui::End();
}

void PlaterRenderModule::on_scene_mouse_event(const Platform::MouseEvent& e)
{
    m_gizmo_manager->on_scene_mouse_event(e, m_screen_info);
}
void PlaterRenderModule::on_scene_keyboard_event(const Platform::KeyboardEvent& e)
{
    if (!m_gizmo_manager->on_scene_keyboard_event(e))
        Platform::AbstractRenderModule::on_scene_keyboard_event(e);
}

void PlaterRenderModule::on_activated()
{
    if (m_scene_presenter != nullptr)
        m_scene_presenter->scene().set_lights(Slic3r::App::global_lighting());
}

void PlaterRenderModule::on_deactivated()
{
    Slic3r::App::set_global_lighting(m_scene_presenter->scene().lights());

    App::BedStore store;
    size_t project_id = m_project_interactor.selected_project_id();
    const auto& project = m_workbench.project(project_id);
    for (const auto& cc : project.config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            store.beds.push_back(Domain::BedRef{cc->id().id, bi->id().id});
        }
    }
    Slic3r::App::set_bed_store(store);
}

void PlaterRenderModule::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection &selection)
{
    m_toolbar_move->set_enabled(selection.empty());
    m_toolbar_rotate->set_enabled(selection.empty());
}

void PlaterRenderModule::on_screen_resized()
{
    // m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    auto viewport = Render::Rect::from(0, 0, m_screen_info);
    m_scene_presenter->screen_resized(viewport);
}

void PlaterRenderModule::on_set_imgui_render() {}

} // namespace Slic3r::App::Plater
