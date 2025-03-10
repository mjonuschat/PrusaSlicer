#include "Slic3r/App/Preview/PreviewRenderModule.hpp"
#include "Slic3r/App/Preview/PreviewCameraGizmo.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperInputData.hpp"

#include <Slic3r/App/libvgcode/ViewerInputData.hpp>
#include <Slic3r/Biz/libpgcode/Processor.hpp>

#include <LibBGCode/core/core.hpp>
#include <LibBGCode/convert/convert.hpp>

#include <boost/nowide/cstdio.hpp>
#include <boost/filesystem/operations.hpp>

#define ENABLED_DEBUG_VIEWER 1

using namespace Slic3r::Biz::libpgcode;
using namespace Slic3r::App::libvgcode;

namespace Slic3r::App::Preview {

void PreviewRenderModule::render_scene()
{
    m_device->load_state();
    auto cmd_buffer = m_device->create_command_buffer();

    cmd_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer->set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer->clear_buffers(true, true);

    m_viewer.render_toolpaths(m_scene_presenter->scene().camera().position().cast<float>());
    m_scene_presenter->render_scene(*cmd_buffer);

    cmd_buffer->submit();
}

#if ENABLED_DEBUG_VIEWER
static void render_imgui_debug_viewer(LibvgcodeWrapper::Wrapper& viewer, Biz::ProjectInteractor& project_interactor)
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

        // Temporary hack
        if (ImGui::Button("Slice all", { 0.0f, 0.0f })) {
            project_interactor.slicing_interactor().slice_all();
        }

    }
    ImGui::End();
}
#endif // ENABLED_DEBUG_VIEWER

void PreviewRenderModule::render_imgui()
{
    LibvgcodeWrapper::WrapperLayoutData layout;
    // TODO: setup layout if needed
    m_viewer.render_gui(layout);

#if ENABLED_DEBUG_VIEWER
    render_imgui_debug_viewer(m_viewer, m_project_interactor);
#endif // ENABLED_DEBUG_VIEWER
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

void PreviewRenderModule::on_fdm_result_cache_changed(
    const Biz::Slicing::SlicingId id
) {
    send_data_to_viewer(m_project_interactor.fdm_result_cache().get_result(id));
}

void PreviewRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    m_scene_presenter =
        std::make_unique<PreviewScenePresenter>(m_workbench, m_project_interactor, *m_device);


    // Temporary (ugly) hack to enable slicing. Remove once switching from plater to
    // preview is possible.
    auto config{m_project_interactor.selected_config_container().print_config()};
    config.set("skirts", 2);
    config.set("brim_width", 10.0);
    config.option<ConfigOptionEnum<DraftShield>>("draft_shield", true)->value = dsEnabled;
    m_project_interactor.selected_config_container().set_print_config(config);
    const double cube1_side{40};
    TriangleMesh cube1{its_make_cube(cube1_side, cube1_side, cube1_side)};
    cube1.translate(30, 0, 0);
    m_project_interactor.scene_interactor().new_object_from_mesh(std::move(cube1));
    const double cube2_side{30};
    TriangleMesh cube2{its_make_cube(cube2_side, cube2_side, cube2_side)};
    cube2.translate(60, 0, 0);
    m_project_interactor.scene_interactor().new_object_from_mesh(std::move(cube2));
    const double cube3_side{80};
    TriangleMesh cube3{its_make_cube(cube3_side, cube3_side, cube3_side)};
    cube3.translate(-80, -60, 0);
    m_project_interactor.scene_interactor().new_object_from_mesh(std::move(cube3));
    const double cube4_side{70};
    TriangleMesh cube4{its_make_cube(cube4_side, cube4_side, cube4_side)};
    cube4.translate(0, 90, 0);
    m_project_interactor.scene_interactor().new_object_from_mesh(std::move(cube4));
    const double cube5_side{50};
    TriangleMesh cube5{its_make_cube(cube5_side, cube5_side, cube5_side)};
    cube5.translate(60, 60, 0);
    m_project_interactor.scene_interactor().new_object_from_mesh(std::move(cube5));



    init_gizmos();
    init_viewer(device);

    Scene::CameraTrackballController& camera_trackball = m_scene_presenter->scene().camera_trackball();
    camera_trackball.set_focal_point(m_viewer.bounding_box().center());
    camera_trackball.set_azimuth_and_zenith(1.25 * PI, 1.25 * PI);
}

void PreviewRenderModule::on_activated()
{
}

void PreviewRenderModule::on_deactivated()
{
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
            new Platform::FuncCommand(
                "toggle-legend-visibility",
                [&]() { m_viewer.toggle_legend_visible(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::L}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "toggle-gcodewindow-visibility",
                [&]() { m_viewer.toggle_gcodewindow_visible(); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::G}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-increase-slow",
                [&]() { m_viewer.slider_gcode_move_current_thumb(1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Right}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-decrease-slow",
                [&]() { m_viewer.slider_gcode_move_current_thumb(-1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Left}
            )
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-increase-medium",
                [&]() { m_viewer.slider_gcode_move_current_thumb(5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Right
                }
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-decrease-medium",
                [&]() { m_viewer.slider_gcode_move_current_thumb(-5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Left
                }
            )
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-increase-fast",
                [&]() { m_viewer.slider_gcode_move_current_thumb(10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Right
                }
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-gcode-decrease-fast",
                [&]() { m_viewer.slider_gcode_move_current_thumb(-10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Left
                }
            )
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-increase-slow",
                [&]() { m_viewer.slider_layers_move_current_thumb(1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Up}
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-decrease-slow",
                [&]() { m_viewer.slider_layers_move_current_thumb(-1); },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Down}
            )
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-increase-medium",
                [&]() { m_viewer.slider_layers_move_current_thumb(5); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Up
                }
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-decrease-medium",
                [&]() { m_viewer.slider_layers_move_current_thumb(-5); },
                nullptr,
                Platform::KeyboardShortcut{ 
                    Platform::KeyModifiers(Platform::KeyModifier::Shift), Platform::KeyCode::Down
                }
            )
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-increase-fast",
                [&]() { m_viewer.slider_layers_move_current_thumb(10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Up
                }
            ),
            true
        )
        .register_command(
            new Platform::FuncCommand(
                "slider-layers-decrease-fast",
                [&]() { m_viewer.slider_layers_move_current_thumb(-10); },
                nullptr,
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Ctrl), Platform::KeyCode::Down
                }
            )
        )
    ;
}

void PreviewRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<Scene::GizmoManager>(*m_device, *m_scene_presenter);
    m_gizmo_manager->add_base_gizmo<PreviewCameraGizmo>(*m_scene_presenter);
}

void PreviewRenderModule::init_viewer(Render::Device& device)
{
    bool is_editor = true;
    bool show_ruler_in_dbl_slider = false;
    bool show_ruler_bg_in_dbl_slider = false;
    bool show_estimated_times_in_dbl_slider = true;

    App::LibvgcodeWrapper::WrapperSettings settings;
    settings.slider_layers_editable = is_editor;
    settings.slider_layers_show_ruler = show_ruler_in_dbl_slider;
    settings.slider_layers_show_ruler_bg = show_ruler_bg_in_dbl_slider;
    settings.slider_layers_show_estimated_times = show_estimated_times_in_dbl_slider;
    settings.settings_in_legend_visible = !is_editor;
    // set wrapper callbacks
    settings.cb_invalidate_slice = std::bind(&PreviewRenderModule::on_invalidate_slice, this);
    settings.cb_update_layers_slider = std::bind(&PreviewRenderModule::on_update_layers_slider, this, std::placeholders::_1);
    settings.cb_request_extra_frames = std::bind(&PreviewRenderModule::on_request_extra_frames, this, std::placeholders::_1);
    settings.cb_gcode_view_type_changed = std::bind(&PreviewRenderModule::on_gcode_view_type_changed, this);
    // set layers slider callbacks
    settings.cb_slider_layers_on_thumb_move = std::bind(&PreviewRenderModule::on_slider_layers_on_thumb_move, this);
    settings.cb_slider_layers_ticks_changed = std::bind(&PreviewRenderModule::on_slider_layers_ticks_changed, this);
    settings.cb_slider_layers_get_extruder_colors = std::bind(&PreviewRenderModule::on_slider_layers_get_extruder_colors, this);
    settings.cb_slider_layers_auto_color_change = std::bind(&PreviewRenderModule::on_slider_layers_auto_color_change, this);
    settings.cb_slider_layers_check_gcode = std::bind(&PreviewRenderModule::on_slider_layers_check_gcode, this, std::placeholders::_1);
    settings.cb_slider_layers_get_extruders_sequence = std::bind(&PreviewRenderModule::on_slider_layers_get_extruders_sequence, this, std::placeholders::_1);
    settings.cb_slider_layers_get_custom_code = std::bind(&PreviewRenderModule::on_slider_layers_get_custom_code, this, std::placeholders::_1, std::placeholders::_2);
    settings.cb_slider_layers_get_pause_print_msg = std::bind(&PreviewRenderModule::on_slider_layers_get_pause_print_msg, this, std::placeholders::_1, std::placeholders::_2);
    settings.cb_slider_layers_get_new_color = std::bind(&PreviewRenderModule::on_slider_layers_get_new_color, this, std::placeholders::_1);
    settings.cb_slider_layers_show_info_msg = std::bind(&PreviewRenderModule::on_slider_layers_show_info_msg, this, std::placeholders::_1, std::placeholders::_2);
    settings.cb_slider_layers_get_gcode = std::bind(&PreviewRenderModule::on_slider_layers_get_gcode, this, std::placeholders::_1);
    settings.cb_slider_layers_get_used_extruders_in_print = std::bind(&PreviewRenderModule::on_slider_layers_get_used_extruders_in_print, this, std::placeholders::_1);
    settings.cb_slider_layers_app_config_changed = std::bind(&PreviewRenderModule::on_slider_layers_app_config_changed, this, std::placeholders::_1, std::placeholders::_2);
    // set gcode slider callbacks
    settings.cb_slider_gcode_on_thumb_move = std::bind(&PreviewRenderModule::on_slider_gcode_on_thumb_move, this);

    if (is_editor) {
        // legend's custom options
        CustomOption& shells_option = settings.custom_options.emplace_back(CustomOption());
        shells_option.name = _u8L("Shells");
        shells_option.icon = ImGui::LegendShells;
        shells_option.cb_action = std::bind(&PreviewRenderModule::on_legend_shells_action, this, std::placeholders::_1);
    }

    if (m_viewer.init(device, m_scene_presenter->scene(), m_gizmo_manager->data_factory(), settings))
        m_viewer.set_lights(m_viewer.default_lights());
    else {
        // log some error message
    }
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
        ret.first = extract_processor_config_from_ankermakestudio_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::BambuStudio:
    {
        ret.first = extract_processor_config_from_bambustudio_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::CraftWare:
    {
        ret.first = extract_processor_config_from_craftware_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::Cura:
    {
        ret.first = extract_processor_config_from_cura_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::KISSlicer:
    {
        ret.first = extract_processor_config_from_kisslicer_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::ideaMaker:
    {
        ret.first = extract_processor_config_from_ideamaker_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::OrcaSlicer:
    {
        ret.first = extract_processor_config_from_orcaslicer_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::PrusaSlicer:
    {
        ret.first = extract_processor_config_from_prusaslicer_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::Simplify3D:
    {
        ret.first = extract_processor_config_from_simplify3d_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::SuperSlicer:
    {
        ret.first = extract_processor_config_from_superslicer_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    case GCodeProducer::XDesktop:
    {
        ret.first = extract_processor_config_from_xdesktop_gcode(ret.second, [](const std::string_view str, size_t* pos = nullptr) {
            return string_to_double_decimal_point(str, pos);
        });
        break;
    }
    default:
    {
        break;
    }
    }

    ret.first.callbacks.cb_string_to_double_decimal_point = [](const std::string_view str, size_t* pos = nullptr) {
        return string_to_double_decimal_point(str, pos);
    };
    ret.first.callbacks.cb_float_to_string_decimal_point = [](double value, int precision = -1) {
        return float_to_string_decimal_point(value, precision);
    };

    return ret;
}

//
// Temporary function for test
//
static ProcessorResult result_from_gcode_file()
{
    // >>> filename of the test file
//    std::string filename = "c:/temp/test.bgcode";
//    std::string filename = "c:/temp/test.gcode";
    std::string filename = "c:/temp/Bařtipán.gcode";

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

//
// Temporary function for test
//
static ViewerInputData extract_viewer_input_data_from_result(ProcessorResult&& result)
{
    ViewerInputData ret;

    std::vector<std::string> str_tool_colors = result.extruder_str_colors;
    std::vector<std::string> str_color_print_colors;
    if (!result.custom_gcode_per_print_z.empty()) {
        str_color_print_colors = result.extruder_str_colors;
        for (const CustomGCode::Item& code : result.custom_gcode_per_print_z) {
            if (code.type == CustomGCode::Type::ColorChange)
                str_color_print_colors.emplace_back(code.color);
        }
        str_color_print_colors.push_back(DUMMY_STR_COLOR);
    }

    ret.tools_colors.reserve(str_tool_colors.size());
    for (const std::string& str_color : str_tool_colors) {
        ColorRGB color;
        decode_color(str_color, color);
        ret.tools_colors.emplace_back(color);
    }

    const std::vector<std::string>& str_colors = str_color_print_colors.empty() ? str_tool_colors : str_color_print_colors;
    ret.color_print_colors.reserve(str_colors.size());
    for (const std::string& str_color : str_colors) {
        ColorRGB color;
        decode_color(str_color, color);
        ret.color_print_colors.emplace_back(color);
    }

    for (const auto& [role, values] : result.print_statistics.used_filaments_per_role) {
        float length = values.first;
        float mass   = values.second;
        ret.used_filament_by_roles.insert({ role, { length, mass } });
    }

    for (const auto& [extruder_id, volume] : result.print_statistics.volumes_per_extruder) {
        float v = 0.001f * volume;
        float length = v / result.filament_geometry(extruder_id).area_cross_section;
        float mass = v * result.filament_densities[extruder_id];
        ret.used_filament_by_extruders.insert({ extruder_id, { length, mass } });
    }

    std::array<size_t, TIME_MODES_COUNT> shifts = {};
    size_t color_changes_count = 0;
    for (size_t i = 0; i < result.custom_gcode_per_print_z.size(); ++i) {
        const auto& item = result.custom_gcode_per_print_z[i];
        assert(item.extruder > 0);
        std::array<float, TIME_MODES_COUNT> times = {};
        std::array<float, 2> used_filament = { 0.0f, 0.0f };
        for (size_t j = 0; j < TIME_MODES_COUNT; ++j) {
            const Biz::libpgcode::PrintEstimatedStatistics::Mode& mode = result.print_statistics.modes[j];
            auto it = std::find_if(mode.custom_gcode_times.begin() + shifts[j], mode.custom_gcode_times.end(),
                [&item](const std::pair<CustomGCode::Type, std::pair<float, float>>& gc_item) { return gc_item.first == item.type; });
            if (it != mode.custom_gcode_times.end()) {
                shifts[j] = std::distance(mode.custom_gcode_times.begin(), it) + 1;
                times[j] = it->second.first;
            }
        }
        if (item.type == CustomGCode::Type::ColorChange) {
            float volume = 0.001f * result.print_statistics.volumes_per_color_change[color_changes_count++];
            used_filament = { volume / result.filament_geometry(uint8_t(item.extruder - 1)).area_cross_section,
                              volume * result.filament_densities[item.extruder - 1] };
        }
        ret.gcode_events.push_back({ item.type, uint8_t(item.extruder - 1), times, used_filament });
    }

    ret.gcode = std::move(result.gcode);
    ret.vertices = std::move(result.moves);

    return ret;
}

//
// Temporary function for test
//
static LibvgcodeWrapper::WrapperInputData extract_wrapper_input_data_from_result(const ProcessorResult& result)
{
    LibvgcodeWrapper::WrapperInputData ret;

    CustomGCode::Info ticks_info_from_model;
    ticks_info_from_model.mode = CustomGCode::Mode::SingleExtruder;
    ticks_info_from_model.gcodes = result.custom_gcode_per_print_z;
    ret.producer = result.producer;
    ret.custom_gcode_info = ticks_info_from_model;
    ret.print_settings = result.print_settings;

    return ret;
}

void PreviewRenderModule::send_data_to_viewer(Biz::Slicing::FDMResult result)
{
    ViewerInputData viewer_data = extract_viewer_input_data_from_result(std::move(result));
    LibvgcodeWrapper::WrapperInputData wrapper_data = extract_wrapper_input_data_from_result(result);

    m_viewer.reset_default_extrusion_roles_colors();
    m_viewer.load(std::move(wrapper_data), std::move(viewer_data));

    // request redraw
    request_render();

    if (!m_viewer.has_data()) {
        // log some error message
        return;
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
    // TODO
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

std::vector<std::string> PreviewRenderModule::on_slider_layers_get_extruder_colors()
{
    // TODO
    return std::vector<std::string>();
}

bool PreviewRenderModule::on_slider_layers_auto_color_change()
{
    // TODO
    return false;
}

void PreviewRenderModule::on_slider_layers_check_gcode(CustomGCode::Type type)
{
    // TODO
}

bool PreviewRenderModule::on_slider_layers_get_extruders_sequence(LibvgcodeWrapper::ExtrudersSequence& sequence)
{
    // TODO
    return false;
}

std::string PreviewRenderModule::on_slider_layers_get_custom_code(const std::string& code_in, float height)
{
    // TODO
    return "";
}

std::string PreviewRenderModule::on_slider_layers_get_pause_print_msg(const std::string& msg_in, float height)
{
    // TODO
    return "";
}

std::string PreviewRenderModule::on_slider_layers_get_new_color(const std::string& color)
{
    // TODO
    return "";
}

int PreviewRenderModule::on_slider_layers_show_info_msg(const std::string& message, int btns_flag)
{
    // TODO
    return 0;
}

std::string PreviewRenderModule::on_slider_layers_get_gcode(CustomGCode::Type type)
{
    // TODO
    return "";
}

std::set<int> PreviewRenderModule::on_slider_layers_get_used_extruders_in_print(float print_z)
{
    // TODO
    return std::set<int>();
}

void PreviewRenderModule::on_slider_layers_app_config_changed(const std::string& key, const std::string& val)
{
    if (key == "seq_top_layer_only") {
        bool active = m_viewer.is_top_layer_only_view_range();
        bool required = val == "1";
        if (active != required) {
            m_viewer.toggle_top_layer_only_view_range();
            const Interval& range = m_viewer.layers_range();
            m_viewer.set_layers_range(range[0], range[1]);
        }
    }
}

void PreviewRenderModule::on_slider_gcode_on_thumb_move()
{
    // TODO
}

void PreviewRenderModule::on_legend_shells_action(bool visible)
{
    // TODO
}

} // namespace Slic3r::App::Preview
