#include "Slic3r/App/Preview/PreviewRenderModule.hpp"

#include "Slic3r/App/Preview/PreviewCameraGizmo.hpp"
#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Preview/FdmViewerWrapperInputData.hpp"
#include "Slic3r/App/Preview/Types.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/IRenderModuleChangedListener.hpp"
#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"
#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Scene/LightingHelper.hpp"
#include "Slic3r/App/LightSetting.hpp"
#include "Slic3r/App/BedThumbnailStore.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"

#include "Slic3r/Domain/TriangleMesh.hpp"

#include <Slic3r/App/libvgcode/FdmViewerInputData.hpp>
#include <Slic3r/Biz/libpgcode/Processor.hpp>

#include <LibBGCode/core/core.hpp>
#include <LibBGCode/convert/convert.hpp>

#include <boost/nowide/cstdio.hpp>
#include <boost/filesystem/operations.hpp>

#define ENABLED_DEBUG_VIEWER 0
#define ENABLED_DEBUG_LOAD_DATA 0
#define ENABLED_DEBUG_VIEWER_MODE 0

using namespace Slic3r::Biz;
using namespace Slic3r::Biz::libpgcode;
using namespace Slic3r::App::libvgcode;
using namespace Slic3r::App::Yoga;

using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec4d;

namespace Slic3r::App::Preview {

using Domain::TriangleMesh;
namespace CustomGCode = Domain::CustomGCode;

void PreviewRenderModule::render_scene(Render::CommandBuffer& cmd_buffer)
{
    Render::ScopedDebugGroup event_imgui_render("Preview Render", cmd_buffer);
    m_device->load_state();

    cmd_buffer.set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer.set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer.clear_buffers(true, true);

    m_viewer->render_scene();
    m_scene_presenter->render_scene(cmd_buffer);

    cmd_buffer.submit();
}

#if ENABLED_DEBUG_VIEWER
static void render_imgui_debug_viewer(Wrapper& viewer)
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Preview debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing)) {

        if (ImGui::Button("COG marker scale factor", { -1.0f, 0.0f }))
            viewer.set_scale_factor_popup_type(Biz::libpgcode::OptionType::CenterOfGravity);
        if (ImGui::Button("Tool marker scale factor", { -1.0f, 0.0f }))
            viewer.set_scale_factor_popup_type(Biz::libpgcode::OptionType::ToolMarker);

        ImGui::Separator();

        if (ImGui::Button("Travels radius", { -1.0f, 0.0f }))
            viewer.set_radius_popup_type(Biz::libpgcode::MoveType::Travel);
        if (ImGui::Button("Wipes radius", { -1.0f, 0.0f }))
            viewer.set_radius_popup_type(Biz::libpgcode::MoveType::Wipe);

        ImGui::Separator();

        if (ImGui::Button("Extrusion roles color", { -1.0f, 0.0f }))
            viewer.set_extrusion_roles_colors_popup_visible(true);
        if (ImGui::Button("Options color", { -1.0f, 0.0f }))
            viewer.set_options_colors_popup_visible(true);

        ImGui::Separator();

        if (ImGui::Button("Height range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::Height);
        if (ImGui::Button("Width range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::Width);
        if (ImGui::Button("Speed range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::Speed);
        if (ImGui::Button("Actual speed range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::ActualSpeed);
        if (ImGui::Button("Fan speed range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::FanSpeed);
        if (ImGui::Button("Temperature range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::Temperature);
        if (ImGui::Button("Volumetric flow rate range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::VolumetricFlowRate);
        if (ImGui::Button("Layer time linear range colors", { -1.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::LayerTimeLinear);
        if (ImGui::Button("Layer time logarithmic range colors", { 0.0f, 0.0f }))
            viewer.set_range_colors_popup_type(libvgcode::ViewType::LayerTimeLogarithmic);
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_VIEWER

#if ENABLED_DEBUG_LOAD_DATA
static void render_imgui_debug_load_data(Wrapper& viewer, Biz::ProjectInteractor& project_interactor,
    std::function<void(const std::string&)> cb)
{
    // currently the call to slice_all() works only the 1st time
    static bool sliced = false;

    ImGui::SetNextWindowPos({ ImGui::GetMainViewport()->GetCenter().x, 2.5f * ImGui::GetTextLineHeight() },
        ImGuiCond_Always, { 0.5f, 0.0f });
    if (ImGui::Begin("Load data", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing)) {

        bool disabled = sliced || viewer.mode() != FdmViewerWrapperMode::EditorGCode;
        if (disabled) ImGui::BeginDisabled();
        ImGui::Spacing();
        if (ImGui::Button("Slice all", { -1.0f, 0.0f })) {
            viewer.reset();
            project_interactor.slicing_interactor().slice_all();
            sliced = true;
        }
        ImGui::Spacing();
        if (disabled) ImGui::EndDisabled();

        ImGui::SeparatorText("Test files");

        const char* files[] = {
            "test.gcode",
            "test.bgcode",
            "test_colors.bgcode",
            "test_v_slider.bgcode",
            "test_multimaterial.gcode",
            "test_sequential.gcode",
            "test_vase.gcode"
        };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Select file");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        static int current_file = 0;
        ImGui::Combo("##modes", &current_file, files, IM_ARRAYSIZE(files));
        if (ImGui::Button("Load", { -1.0f, 0.0f }))
            cb(files[current_file]);
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_LOAD_DATA

#if ENABLED_DEBUG_VIEWER_MODE
static void render_imgui_debug_viewer_mode(Wrapper& viewer)
{
    ImGui::SetNextWindowPos({ ImGui::GetMainViewport()->GetCenter().x, 0.0f }, ImGuiCond_Always, { 0.5f, 0.0f });
    if (ImGui::Begin("Type debug", nullptr, ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBackground)) {

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mode:");
        ImGui::SameLine();

        const char* items[] = { "EditorGCode", "GCodeViewer" };
        static int item_current = 0;
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::Combo("##modes", &item_current, items, IM_ARRAYSIZE(items))) {
            viewer.reset();
            viewer.set_mode((item_current == 0) ? FdmViewerWrapperMode::EditorGCode : FdmViewerWrapperMode::GCodeViewer);
        }

        ImGui::SameLine();
        ImGui::Text("Current:");
        std::string mode_txt;
        switch (viewer.mode())
        {
        case FdmViewerWrapperMode::EditorGCode:    { mode_txt = "EditorGCode"; break; }
        case FdmViewerWrapperMode::EditorPreGCode: { mode_txt = "EditorPreGCode"; break; }
        case FdmViewerWrapperMode::EditorSLA:      { mode_txt = "EditorSLA"; break; }
        case FdmViewerWrapperMode::GCodeViewer:    { mode_txt = "GCodeViewer"; break; }
        default:                          { mode_txt = "Error"; break; }
        }
        ImGui::SameLine();
        ImGui::Text("%s", mode_txt.c_str());
    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_VIEWER_MODE


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


void PreviewRenderModule::render_imgui(Render::CommandBuffer& cmd_buffer)
{
    // temporary to allow to switch yoga layout on/off
    if (m_use_yoga_layout) {
        bool gcode_window_enabled = m_fdm_viewer.mode() != FdmViewerWrapperMode::EditorPreGCode && m_fdm_viewer.has_data() &&
            m_gcode_window->is_visible();

        if (m_layout) {
            m_gcode_window->set_visible(gcode_window_enabled);
            m_slider_layers->set_visible(m_fdm_viewer.has_data());
            m_sla_slider_layers->set_visible(m_sla_viewer.has_data());
            m_slider_gcode->set_visible(m_fdm_viewer.has_data());
        }

        m_cube_view->set_camera_data(m_scene_presenter->scene().camera(), m_scene_presenter->scene().camera_trackball());

        m_layout->render({ m_screen_info.logical_width(), m_screen_info.logical_height() });

        if (m_cube_view->require_render())
            request_render();

        m_fdm_viewer.set_tool_marker_enabled(gcode_window_enabled);
    }
    else {
        const libvgcode::Interval& visible_range = m_fdm_viewer.view_visible_range();
        const libvgcode::Interval& enabled_range = m_fdm_viewer.view_enabled_range();
        bool gcode_window_enabled = m_fdm_viewer.mode() != FdmViewerWrapperMode::EditorPreGCode && m_fdm_viewer.has_data() &&
            visible_range[1] != enabled_range[1];
        m_fdm_viewer.set_tool_marker_enabled(gcode_window_enabled);
        WrapperLayoutData layout;
        // TODO: setup layout if needed
        m_fdm_viewer.render_gui(layout);
    }

if (ImGui::Begin("Outline", nullptr)) {
        imgui_scenegraph_node_info(m_scene_presenter->scene().root());
    }
    ImGui::End();

#if ENABLED_DEBUG_VIEWER
    render_imgui_debug_viewer(m_fdm_viewer);
#endif // ENABLED_DEBUG_VIEWER
#if ENABLED_DEBUG_LOAD_DATA
    render_imgui_debug_load_data(m_fdm_viewer, m_project_interactor, [this](const std::string& filename) {
        m_fdm_viewer.reset();
        send_data_to_viewer_from_file(Slic3r::resources_dir() + "/test_data/" + filename);
        m_layout->layout_toolbars_sizer();
    });
#endif // ENABLED_DEBUG_LOAD_DATA
#if ENABLED_DEBUG_VIEWER_MODE
    render_imgui_debug_viewer_mode(m_fdm_viewer);
#endif // ENABLED_DEBUG_VIEWER_MODE
#if ENABLED_SCENE_SHADING_CUSTOMIZATION
    render_imgui_scene_shading_customization(*m_scene_presenter);
#endif // ENABLED_SCENE_SHADING_CUSTOMIZATION
#if ENABLED_LIGHTS_CUSTOMIZATION
    render_imgui_lights_customization(*m_scene_presenter);
#endif // ENABLED_LIGHTS_CUSTOMIZATION
}

void PreviewRenderModule::on_scene_mouse_event(const Platform::MouseEvent& e)
{
    m_gizmo_manager->on_scene_mouse_event(e, m_screen_info);
}

void PreviewRenderModule::on_scene_keyboard_event(const Platform::KeyboardEvent& e)
{
    if (!m_gizmo_manager->on_scene_keyboard_event(e))
      Platform::AbstractRenderModule::on_scene_keyboard_event(e);
}

void PreviewRenderModule::add_type_changed_listener(IRenderModuleChangedListener* l)
{
    m_render_module_changed_listeners.insert(l);
}

void PreviewRenderModule::remove_type_changed_listener(IRenderModuleChangedListener* l)
{
    m_render_module_changed_listeners.erase(l);
}

void PreviewRenderModule::on_selected_bed_instance_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId container_id,
    Domain::SelectionId bed_instance_id)
{
    DEBUG_ASSERT(m_project_interactor.selected_project_id() == project_id);

    const Domain::ConfigContainer* cc = m_project_interactor.selected_project().find_config_container(container_id);
    DEBUG_ASSERT(cc != nullptr);
    if (cc->print_technology() == Domain::PrinterTechnology::SLA) {
        m_viewer = &m_sla_viewer;
        update_sla_viewer_data({ project_id, bed_instance_id });
    }
    else {
        m_viewer = &m_fdm_viewer;
        update_fdm_viewer_data({ project_id, bed_instance_id });
    }

    m_scene_presenter->update_bed_instances();
    center_camera_on_selected_bed();

    request_render();
}

void PreviewRenderModule::on_status_cache_changed(const Biz::Slicing::SlicingId id)
{
/*    if (m_project_interactor.selected_project_id() == id.project_id && m_viewer->has_data()) {
        const std::optional<Biz::Slicing::Status> status {
            m_project_interactor.status_cache().get_status(id) };
        if (status && status == Biz::Slicing::Status::Modified)
            m_viewer->reset();
    }*/
    m_object_list->update_sliced_info();

    // request redraw
    request_render();
}

void PreviewRenderModule::on_selected_project_changed(size_t index)
{
    update_bed_instances();
}

void PreviewRenderModule::set_sidebars_visible(bool hide)
{
    m_layout->set_sidebars_visible(hide);
    // request redraw
    request_render();
}

void PreviewRenderModule::on_init(Render::Device& device, Render::ImguiRender& imgui_render)
{

    AbstractRenderModule::on_init(device, imgui_render);
    Yoga::Item::set_imgui_render(&imgui_render); // Todo: move this somewhere where it is invoked once
    m_scene_presenter =
        std::make_unique<PreviewScenePresenter>(m_workbench, m_project_interactor, *m_device);

    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstanceChangedListener>( this );
    m_project_interactor.fdm_result_cache().add_listener<Biz::IFDMResultCacheChangedListener>( this );
    m_project_interactor.sla_result_cache().add_listener<Biz::ISLAResultCacheChangedListener>( this );
    m_project_interactor.sla_object_cache().add_listener<Biz::ISLAObjectCacheChangedListener>( this );
    m_project_interactor.status_cache().add_listener<Biz::IStatusCacheChangedListener>( this );
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);

    init_gizmos();
    init_viewers(device);
    init_scene_layout();

    m_scene_presenter->scene().set_lights(Slic3r::App::global_lighting());

    // select active viewer with respect to the printer technology of selected config container
    Domain::SelectionId config_container_id = m_project_interactor.scene_interactor().selected_config_container_id();
    const Domain::ConfigContainer* cc = m_project_interactor.selected_project().find_config_container(config_container_id);
    DEBUG_ASSERT(cc != nullptr);
    if (cc->print_technology() == Domain::PrinterTechnology::SLA)
        m_viewer = &m_sla_viewer;
    else
        m_viewer = &m_fdm_viewer;
}

void PreviewRenderModule::on_activated()
{
    if (m_scene_presenter != nullptr)
        m_scene_presenter->scene().set_lights(App::global_lighting());

    update_bed_instances();
    center_camera_on_selected_bed();
}

void PreviewRenderModule::on_deactivated()
{
    Slic3r::App::set_global_lighting(m_scene_presenter->scene().lights());
}

void PreviewRenderModule::on_screen_resized()
{
    //m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    auto viewport = Render::Rect::from(0, 0, m_screen_info);
    m_scene_presenter->screen_resized(viewport);
}

void PreviewRenderModule::register_commands()
{
    m_command_registry
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "toggle-legend-visibility",
                [this]() { m_button_legend->callbacks().action(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::L}
                )
            )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "toggle-gcodewindow-visibility",
                [this]() { m_button_gcode->callbacks().action(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::G}
                )
            );

    m_command_registry
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-increase-slow",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Right}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-decrease-slow",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(-1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Left}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-increase-medium",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Right
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-decrease-medium",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(-5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Left
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-increase-fast",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Right
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-gcode-decrease-fast",
                [this]() { m_fdm_viewer.slider_gcode_move_current_thumb(-10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Left
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-increase-slow",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Up}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-decrease-slow",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(-1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Down}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-increase-medium",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Up
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-decrease-medium",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(-5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Down
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-increase-fast",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Up
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-decrease-fast",
                [this]() { m_fdm_viewer.slider_layers_move_current_thumb(-10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Down
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-jump-to_value",
                [this]() { m_fdm_viewer.slider_layers_jump_to_value(); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::G
                }
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-add-current-tick",
                [this]() { m_fdm_viewer.slider_layers_add_current_tick(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Plus}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-add-current-tick-kp",
                [this]() { m_fdm_viewer.slider_layers_add_current_tick(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::KpPlus}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-delete-current-tick",
                [this]() { m_fdm_viewer.slider_layers_delete_current_tick(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Minus}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "slider-layers-delete-current-tick-kp",
                [this]() { m_fdm_viewer.slider_layers_delete_current_tick(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::KpMinus}
            )
        )
        // temporary to allow to switch yoga layout on/off
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "use-yoga-layout",
                [this]() { m_use_yoga_layout = !m_use_yoga_layout; },
                nullptr,
                Platform::KeyboardShortcut{ 0, Platform::KeyCode::Y }
            )
        )
        ;
}

void PreviewRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<Scene::GizmoManager>(*m_device, *m_scene_presenter, m_project_interactor);
    m_gizmo_manager->add_base_gizmo<PreviewCameraGizmo>(m_workbench, *m_scene_presenter);
}

void PreviewRenderModule::init_viewers(Render::Device& device)
{
    // the following values should be taken from the app.ini config
    bool show_ruler_in_dbl_slider = false;
    bool show_ruler_bg_in_dbl_slider = false;
    bool show_estimated_times_in_dbl_slider = true;
    bool use_default_colors_in_dbl_slider = false;
    bool seq_top_layer_only = false;

    // Initialize the SLA ViewerWrapper

    ViewerWrapperBaseSettings base_settings;
    base_settings.slider_layers_show_ruler = show_ruler_in_dbl_slider;
    base_settings.slider_layers_show_ruler_bg = show_ruler_bg_in_dbl_slider;
    base_settings.slider_layers_show_estimated_times = show_estimated_times_in_dbl_slider;
    // set layers slider callbacks
    base_settings.cb_slider_layers_on_thumb_move = std::bind(&PreviewRenderModule::on_slider_layers_on_thumb_move, this);

    if (m_sla_viewer.init(device, m_scene_presenter->scene(), m_gizmo_manager->data_factory()) &&
        m_sla_viewer.set_settings(base_settings)) {
        m_sla_viewer.set_lights(Slic3r::App::global_lighting());

        m_sla_slider_layers = Passthrough(m_sla_viewer.unload_double_slider_layers());
    }
    else {
        // log some error message
    }

    // Initialize the FDM ViewerWrapper

    FdmViewerWrapperMode mode = FdmViewerWrapperMode::EditorGCode;

    FdmViewerWrapperSettings settings;
    settings.ViewerWrapperBaseSettings::operator=(base_settings);

    settings.mode = mode;
    settings.slider_layers_use_default_colors = use_default_colors_in_dbl_slider;
    settings.seq_top_layer_only = seq_top_layer_only;
    // set wrapper callbacks
    settings.cb_invalidate_slice = std::bind(&PreviewRenderModule::on_invalidate_slice, this);
    settings.cb_update_layers_slider = std::bind(&PreviewRenderModule::on_update_layers_slider, this, std::placeholders::_1);
    settings.cb_request_extra_frames = std::bind(&PreviewRenderModule::on_request_extra_frames, this, std::placeholders::_1);
    settings.cb_gcode_view_type_changed = std::bind(&PreviewRenderModule::on_gcode_view_type_changed, this);
    // set layers slider callbacks
    settings.cb_slider_layers_on_thumb_move = std::bind(&PreviewRenderModule::on_slider_layers_on_thumb_move, this);
    settings.cb_slider_layers_ticks_changed = std::bind(&PreviewRenderModule::on_slider_layers_ticks_changed, this);
    settings.cb_slider_layers_auto_color_change = std::bind(&PreviewRenderModule::on_slider_layers_auto_color_change, this);
    settings.cb_slider_layers_notify_empty_auto_color_change = std::bind(&PreviewRenderModule::on_slider_layers_notify_empty_auto_color_change, this);
    settings.cb_slider_layers_notify_empty_color_change_gcode = std::bind(&PreviewRenderModule::on_slider_layers_notify_empty_color_change_gcode, this);
    settings.cb_slider_layers_get_extruders_sequence = std::bind(&PreviewRenderModule::on_slider_layers_get_extruders_sequence, this, std::placeholders::_1);
    settings.cb_slider_layers_show_info_msg = std::bind(&PreviewRenderModule::on_slider_layers_show_info_msg, this, std::placeholders::_1, std::placeholders::_2);
    settings.cb_slider_layers_get_used_extruders_in_print = std::bind(&PreviewRenderModule::on_slider_layers_get_used_extruders_in_print, this, std::placeholders::_1);
    settings.cb_slider_layers_app_config_changed = std::bind(&PreviewRenderModule::on_slider_layers_app_config_changed, this, std::placeholders::_1, std::placeholders::_2);
    // set gcode slider callbacks
    settings.cb_slider_gcode_on_thumb_move = std::bind(&PreviewRenderModule::on_slider_gcode_on_thumb_move, this);

    if (mode == FdmViewerWrapperMode::EditorGCode || mode == FdmViewerWrapperMode::EditorPreGCode) {
        // legend's custom options
        CustomOption& shells_option = settings.custom_options.emplace_back(CustomOption());
        shells_option.name = _u8L("Shells");
        shells_option.icon = Render::Icon::LegendShells;
        shells_option.cb_action = std::bind(&PreviewRenderModule::on_legend_shells_action, this, std::placeholders::_1);
    }

    if (m_fdm_viewer.init(device, m_scene_presenter->scene(), m_gizmo_manager->data_factory()) && m_fdm_viewer.set_settings(settings)) {
        m_fdm_viewer.set_lights(Slic3r::App::global_lighting());

        m_legend = Passthrough(m_fdm_viewer.unload_legend());
        m_gcode_window = Passthrough(m_fdm_viewer.unload_gcode_window());
        m_slider_gcode = Passthrough(m_fdm_viewer.unload_double_slider_gcode());
        m_slider_layers = Passthrough(m_fdm_viewer.unload_double_slider_layers());
    }
    else {
        // log some error message
    }
}

void PreviewRenderModule::init_scene_layout()
{
    // >> This code is same for Plater/PreviewRenderModule
    m_top_bar = std::make_unique<TopBar>(&m_project_interactor, this);

    m_object_list = Passthrough(std::make_unique<ObjectListWindow>(&m_project_interactor, false));

    m_cube_view = std::make_unique<CubeView>();
    m_sidebar_bed = std::make_unique<SidebarBed>();
    m_sidebar_print = std::make_unique<SidebarPrint>();
    m_sidebar_auto_reslice = std::make_unique<SidebarAutoReslice>();

    m_sidebar_action_buttons = std::make_unique<SidebarPreviewActionButtons>();
    m_sidebar_action_buttons->on_init(&m_project_interactor);
    for (IRenderModuleChangedListener* listener : std::as_const(m_render_module_changed_listeners)) {
        m_sidebar_action_buttons->add_listener<IRenderModuleChangedListener>(listener);
    }

    m_layout.reset(new PreviewRenderLayout(m_top_bar.release(),
                                           m_object_list.release(),
                                           m_cube_view.release(),
                                           m_sidebar_bed.release(),
                                           m_sidebar_print.release(),
                                           m_sidebar_action_buttons.release(),
                                           m_gcode_window.release(),
                                           m_legend.release(),
                                           m_slider_layers.release(),
                                           m_sla_slider_layers.release(),
                                           m_slider_gcode.release(),
                                           m_sidebar_auto_reslice.release()));
    m_layout->init();

    m_button_travels = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendTravel, to_string(OptionType::Travels), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::Travels); }
    }, m_fdm_viewer.is_option_visible(OptionType::Travels));

    m_button_wipes = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendWipe, to_string(OptionType::Wipes), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::Wipes); }
    }, m_fdm_viewer.is_option_visible(OptionType::Wipes));

    m_button_retractions = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendRetract, to_string(OptionType::Retractions), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::Retractions); }
    }, m_fdm_viewer.is_option_visible(OptionType::Retractions));

    m_button_unretractions = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendDeretract, to_string(OptionType::Unretractions), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::Unretractions); }
    }, m_fdm_viewer.is_option_visible(OptionType::Unretractions));

    m_button_seams = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendSeams, to_string(OptionType::Seams), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::Seams); }
    }, m_fdm_viewer.is_option_visible(OptionType::Seams));

    m_button_tool_changes = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendToolChanges, to_string(OptionType::ToolChanges), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::ToolChanges); }
    }, m_fdm_viewer.is_option_visible(OptionType::ToolChanges));

    m_button_color_changes = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendColorChanges, to_string(OptionType::ColorChanges), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::ColorChanges); }
    }, m_fdm_viewer.is_option_visible(OptionType::ColorChanges));

    m_button_pause_prints = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendPausePrints, to_string(OptionType::PausePrints), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::PausePrints); }
    }, m_fdm_viewer.is_option_visible(OptionType::PausePrints));

    m_button_custom_gcodes = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendCustomGCodes, to_string(OptionType::CustomGCodes), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::CustomGCodes); }
    }, m_fdm_viewer.is_option_visible(OptionType::CustomGCodes));

    m_button_center_of_gravity = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendCOG, to_string(OptionType::CenterOfGravity), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::CenterOfGravity); }
    }, m_fdm_viewer.is_option_visible(OptionType::CenterOfGravity));

    m_button_tool_marker = m_layout->add_toolbar_item_checkable(ToolbarID::Middle, Render::Icon::LegendToolMarker, to_string(OptionType::ToolMarker), "", {
        .action     = [this]() { m_fdm_viewer.toggle_option_visibility(OptionType::ToolMarker); }
    }, m_fdm_viewer.is_option_visible(OptionType::ToolMarker));

    m_button_shells = m_layout->add_toolbar_item(ToolbarID::Middle, Render::Icon::LegendShells, "Shells", "", {
        .action     = [this]() { /* TODO */ },
        .checked_changed    = [this](bool checked) { return m_viewer == &m_fdm_viewer && m_fdm_viewer.mode() != FdmViewerWrapperMode::GCodeViewer; }
    });

    // m_layout->set_layer_slider_render_fn([this](Vec2f size, Vec2f pos) {
    //     ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));
    //     const std::string label = _u8L("Layers");
    //     float offset = size.x() - ImGui::CalcTextSize(label.c_str()).x;
    //     if (offset > 0.0f) {
    //         ImGui::Dummy({ 0.5f * offset, ImGui::GetTextLineHeight() });
    //         ImGui::SameLine(0.0f, 0.0f);
    //     }
    //     ImGui::Text("%s", label.c_str());
    //     ImGui::PopFont();
    //     m_viewer.render_layers_slider();
    // });

    // m_layout->set_gcode_slider_render_fn([this](Vec2f size, Vec2f pos) {
    //     ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));
    //     ImGui::BeginGroup();
    //     const std::string label = _u8L("Steps");
    //     float offset = size.y() - ImGui::GetTextLineHeight();
    //     if (offset > 0.0f)
    //         ImGui::Dummy({ ImGui::CalcTextSize(label.c_str()).x, 0.5f * offset });
    //     ImGui::Text("%s", label.c_str());
    //     ImGui::PopFont();
    //     ImGui::EndGroup();
    //     ImGui::SameLine();
    //     m_viewer.render_gcode_slider();
    // });

    // init toolbars

    m_layout->add_toolbar_item_panel(ToolbarID::Top, Render::Icon::ToolbarObjects, "Object List", "Ctrl + Alt + O", {}, m_object_list.get());

    m_button_legend = m_layout->add_toolbar_item_panel(ToolbarID::Bottom, Render::Icon::ToolbarGraph, "Legend", "", {}, m_legend.get()
        // .action = [this]() { m_viewer->toggle_legend_visible(); },
        // .toggled    = [this]() { return m_viewer->has_data() && m_viewer->is_legend_shown(); }
    );

    m_button_gcode = m_layout->add_toolbar_item_panel(ToolbarID::Bottom, Render::Icon::ToolbarGCode, "G-code", "", {}, m_gcode_window.get()
        //.action     = [this]() { m_fdm_viewer.toggle_gcodewindow_visible(); },
        // .toggled    = [this]() { return m_fdm_viewer.has_data() && !m_fdm_viewer.is_gcodewindow_visible(); }
    );

    // Initialize toolbar buttons visibility
    update_toolbar_visibility();
    // <<
}

//
// Temporary function for test
//
static std::pair<ProcessorConfig, std::string> extract_from_gcode(const std::string& filename)
{
    std::pair<ProcessorConfig, std::string> ret;

    FILE* in = boost::nowide::fopen(filename.data(), "rb");
    if (in != nullptr) {
        fseek(in, 0, SEEK_END);
        const long file_size = ftell(in);
        rewind(in);

        if (file_size == 0) {
            fclose(in);
            return ret;
        }

        ret.second.resize(file_size, '\0');
        const std::size_t cnt_read = fread(ret.second.data(), 1, ret.second.size(), in);
        if (cnt_read != ret.second.size()) {
            fclose(in);
            return ret;
        }
    }
    else {
        fclose(in);
        return ret;
    }

    fclose(in);

    const GCodeProducer producer = detect_producer(ret.second);

    switch (producer)
    {
    case GCodeProducer::AnkerMakeStudio:
    {
        ret.first = extract_processor_config_from_ankermakestudio_gcode(ret.second);
        break;
    }
    case GCodeProducer::BambuStudio:
    {
        ret.first = extract_processor_config_from_bambustudio_gcode(ret.second);
        break;
    }
    case GCodeProducer::CraftWare:
    {
        ret.first = extract_processor_config_from_craftware_gcode(ret.second);
        break;
    }
    case GCodeProducer::Cura:
    {
        ret.first = extract_processor_config_from_cura_gcode(ret.second);
        break;
    }
    case GCodeProducer::KISSlicer:
    {
        ret.first = extract_processor_config_from_kisslicer_gcode(ret.second);
        break;
    }
    case GCodeProducer::ideaMaker:
    {
        ret.first = extract_processor_config_from_ideamaker_gcode(ret.second);
        break;
    }
    case GCodeProducer::OrcaSlicer:
    {
        ret.first = extract_processor_config_from_orcaslicer_gcode(ret.second);
        break;
    }
    case GCodeProducer::PrusaSlicer:
    {
        ret.first = extract_processor_config_from_prusaslicer_gcode(ret.second);
        break;
    }
    case GCodeProducer::Simplify3D:
    {
        ret.first = extract_processor_config_from_simplify3d_gcode(ret.second);
        break;
    }
    case GCodeProducer::SuperSlicer:
    {
        ret.first = extract_processor_config_from_superslicer_gcode(ret.second);
        break;
    }
    case GCodeProducer::XDesktop:
    {
        ret.first = extract_processor_config_from_xdesktop_gcode(ret.second);
        break;
    }
    default:
    {
        break;
    }
    }

    return ret;
}

//
// Temporary function for test
//
static ProcessorResult result_from_gcode_file(const std::string& filename)
{
    FILE* in = boost::nowide::fopen(filename.data(), "rb");
    if (in == nullptr) {
        assert(false);
        return ProcessorResult();
    }

    std::vector<std::byte> cs_buffer(65536);
    bool is_binary = bgcode::core::is_valid_binary_gcode(*in, true, cs_buffer.data(), cs_buffer.size()) == bgcode::core::EResult::Success;
    boost::filesystem::path process_filename;
    if (is_binary) {
        // convert the binary gcode into an ASCII temporary file to be used by the libvgcode to populate the gcode window
        boost::filesystem::path output_filename(filename);
        output_filename.replace_extension("tmp.gcode");

        process_filename = output_filename;

        FILE* out = boost::nowide::fopen(process_filename.string().c_str(), "wb");
        if (out == nullptr) {
            assert(false);
            fclose(in);
            return ProcessorResult();
        }

        const bgcode::core::EResult res = bgcode::convert::from_binary_to_ascii(*in, *out, true);
        if (res != bgcode::core::EResult::Success) {
            assert(false);
            fclose(out);
            boost::filesystem::remove(process_filename);
            fclose(in);
            return ProcessorResult();
        }
        fclose(out);
    }
    else
        process_filename = boost::filesystem::path(filename);

    fclose(in);

    std::pair<ProcessorConfig, std::string> processor_data = extract_from_gcode(process_filename.string());
    if (is_binary)
        boost::filesystem::remove(process_filename);

    Processor processor(std::move(processor_data.first));
    processor.process_buffer(std::move(processor_data).second);
    return processor.finalize();
}

void PreviewRenderModule::update_toolbar_visibility()
{
    const OptionTypes& options = m_fdm_viewer.options();
    auto fdm_has_option = [&](OptionType type) {
        return std::find(options.begin(), options.end(), type) != options.end();
    };
    const bool fdm_has_gcode = m_fdm_viewer.has_data() && m_fdm_viewer.mode() != FdmViewerWrapperMode::EditorPreGCode;

    m_button_travels->set_visible(fdm_has_gcode && fdm_has_option(OptionType::Travels));
    m_button_retractions->set_visible(fdm_has_gcode && fdm_has_option(OptionType::Retractions));
    m_button_unretractions->set_visible(fdm_has_gcode && fdm_has_option(OptionType::Unretractions));
    m_button_seams->set_visible(fdm_has_gcode && fdm_has_option(OptionType::Seams));
    m_button_tool_changes->set_visible(fdm_has_gcode && fdm_has_option(OptionType::ToolChanges));
    m_button_color_changes->set_visible(fdm_has_gcode && fdm_has_option(OptionType::ColorChanges));
    m_button_pause_prints->set_visible(fdm_has_gcode && fdm_has_option(OptionType::PausePrints));
    m_button_custom_gcodes->set_visible(fdm_has_gcode && fdm_has_option(OptionType::CustomGCodes));
    m_button_center_of_gravity->set_visible(fdm_has_gcode);
    m_button_tool_marker->set_visible(fdm_has_gcode);
    m_button_shells->set_visible(fdm_has_gcode/*m_fdm_viewer.mode() != FdmViewerWrapperMode::GCodeViewer*/);
    m_button_wipes->set_visible(fdm_has_gcode);

    m_button_legend->set_visible(m_fdm_viewer.has_data() && m_fdm_viewer.mode() == FdmViewerWrapperMode::EditorGCode);
    m_button_gcode->set_visible(fdm_has_gcode);

    m_layout->set_bottom_toolbar_visible(m_button_legend->is_visible() || m_button_gcode->is_visible());
}

void PreviewRenderModule::update_fdm_viewer_data(const Biz::Slicing::SlicingId id)
{
    if (m_project_interactor.selected_bed_slicing_id() != id)
        return;

    const std::optional<Biz::FDMResultRef> fdm_result{ m_project_interactor.fdm_result_cache().get_result(id) };
    if (!fdm_result) {
        m_fdm_viewer.reset();
        return;
    }

    m_fdm_viewer.load_from_result(*fdm_result);

    update_toolbar_visibility();

    // request redraw
    request_render();

    if (!m_fdm_viewer.has_data()) {
        // log some error message
        return;
    }

    const GCodeEvents& gcode_events = m_fdm_viewer.gcode_events();
    m_fdm_viewer.set_view_type(m_fdm_viewer.used_extruders_count() > 1 ? ViewType::Tool :
        gcode_events.empty() ? ViewType::FeatureType : ViewType::ColorPrint);

    center_camera_on_selected_bed();
}

void PreviewRenderModule::update_sla_viewer_result_data(const Biz::Slicing::SlicingId id)
{
    if (m_project_interactor.selected_bed_slicing_id() != id)
        return;
    const std::optional<Biz::SLAResultRef> sla_result{ m_project_interactor.sla_result_cache().get_result(id) };
    if (!sla_result)
        return;

    m_sla_viewer.load_from_result(sla_result->get());
}

void PreviewRenderModule::update_sla_viewer_object_data(const Biz::Slicing::SlicingId id, Domain::ObjectID instance_id)
{
    if (m_project_interactor.selected_bed_slicing_id() != id)
        return;

    const std::optional<Biz::SLAObjectRef> sla_object_result{ m_project_interactor.sla_object_cache().get_instance({id, instance_id}) };
    if (sla_object_result)
        m_sla_viewer.load_from_object(sla_object_result->get());
    else
        m_sla_viewer.reset_from_object(sla_object_result->get());
}

void PreviewRenderModule::update_sla_viewer_data(const Biz::Slicing::SlicingId id)
{
    if (m_project_interactor.selected_bed_slicing_id() != id)
        return;

    m_sla_viewer.reset();
    update_sla_viewer_result_data(id);

    const std::vector<Slic3r::Domain::ObjectID> object_ids = m_project_interactor.sla_object_cache().get_object_ids(id);

    for (const Slic3r::Domain::ObjectID& obj_id : object_ids) {
        update_sla_viewer_object_data(id, obj_id);
    }
}

void PreviewRenderModule::on_invalidate_slice()
{
    // TODO
}

void PreviewRenderModule::on_update_layers_slider(const CustomGCode::Info& info)
{
    // TODO
}

void PreviewRenderModule::on_request_extra_frames(unsigned int count)
{
    for (unsigned int i = 0; i < count; ++i) {
        request_render();
    }
}

void PreviewRenderModule::on_gcode_view_type_changed()
{
    // TODO
}

void PreviewRenderModule::on_slider_layers_on_thumb_move()
{
    // TODO
}

void PreviewRenderModule::on_slider_layers_ticks_changed()
{
    // TODO
}

bool PreviewRenderModule::on_slider_layers_auto_color_change()
{
    // TODO collect data from print config and objects, and update layers slider ticks
    // See:
    // master:                TickCodeManager::auto_color_change()
    // lm_processor_squashed: Preview::layers_slider_auto_color_changed_callback()
    return true;
}

void PreviewRenderModule::on_slider_layers_notify_empty_auto_color_change()
{
    // TODO -> fire notification NotificationType::EmptyAutoColorChange
}

void PreviewRenderModule::on_slider_layers_notify_empty_color_change_gcode()
{
    // TODO -> fire notification NotificationType::EmptyColorChangeCode
}

bool PreviewRenderModule::on_slider_layers_get_extruders_sequence(ExtrudersSequence& sequence)
{
    // TODO
    return false;
}

int PreviewRenderModule::on_slider_layers_show_info_msg(const std::string& message, int btns_flag)
{
    // TODO
    return 0;
}

std::set<int> PreviewRenderModule::on_slider_layers_get_used_extruders_in_print(float print_z)
{
    // TODO: replace the following code with some using ToolOrdering,
    // see master: TickCodeManager::get_used_extruders_for_tick()

    std::vector<uint8_t> ids = m_fdm_viewer.used_extruders_ids();
    std::set<int> ret;
    std::transform(ids.begin(), ids.end(), std::inserter(ret, ret.begin()), [](uint8_t id) {
        return id + 1;
    });

    return ret;
}

void PreviewRenderModule::on_slider_layers_app_config_changed(const std::string& key, const std::string& val)
{
    if (key == "seq_top_layer_only") {
        bool active = m_fdm_viewer.is_top_layer_only_view_range();
        bool required = val == "1";
        if (active != required) {
            m_fdm_viewer.toggle_top_layer_only_view_range();
            const Interval& range = m_fdm_viewer.layers_range();
            m_fdm_viewer.set_layers_range(range[0], range[1]);
        }
    }

    // TODO: update app config
}

void PreviewRenderModule::on_slider_gcode_on_thumb_move()
{
    // TODO
}

void PreviewRenderModule::on_legend_shells_action(bool visible)
{
    // TODO
}

void PreviewRenderModule::center_camera_on_selected_bed()
{
    const Domain::BedInstance* bed_inst = Domain::find_by_id(
        m_project_interactor.selected_config_container().bed_instances(),
        m_project_interactor.scene_interactor().selected_bed_instance().instance_id
    );
    if (bed_inst == nullptr)
        return;

    Domain::Vec3d bed_inst_offset = bed_inst->transformation.get_offset();
    const Domain::Bed& bed = bed_inst->bed;
    std::vector<Domain::Vec3f> print_volume = Biz::Scene::BedGeometry::print_volume(bed);
    Eigen::AlignedBox3d bed_aabb;
    for (const auto& v : print_volume) {
        bed_aabb.extend(bed_inst_offset + v.cast<double>());
    }
    m_scene_presenter->scene().set_shadows_aabb(bed_aabb);
    Scene::CameraTrackballController& trackball = m_scene_presenter->scene().camera_trackball();

    trackball.set_target(bed_inst_offset + Algorithms::Point::to_3d(bed.center(), 0.0));
    trackball.synchronize_pivot_with_target();
}

void PreviewRenderModule::update_bed_instances()
{
    m_scene_presenter->remove_all_bed_instances();
    Plater::BedThumbnailTextures& thumbnails = m_thumbnail_store->projects.selected().thumbnails;
    Domain::BedRefs beds;
    beds.reserve(thumbnails.size());
    for (const auto& t : thumbnails) {
        beds.emplace_back(t.bed_ref);
    }
    m_scene_presenter->add_bed_instances(beds);
    m_scene_presenter->update_bed_instances();
    m_object_list->set_bed_instance_icons(thumbnails);
}

} // namespace Slic3r::App::Preview
