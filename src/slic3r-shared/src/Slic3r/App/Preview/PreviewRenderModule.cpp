#include "Slic3r/App/Preview/PreviewRenderModule.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperInputData.hpp"

#include <Slic3r/App/libvgcode/ViewerInputData.hpp>
#include <Slic3r/Biz/libpgcode/ProcessorResult.hpp>

#include <boost/nowide/cstdio.hpp>

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

    m_scene_presenter->render_scene(*cmd_buffer);

    cmd_buffer->submit();
}

void PreviewRenderModule::render_imgui()
{
    auto& camera = m_scene_presenter->scene().camera();

    LibvgcodeWrapper::WrapperLayoutData layout;
    Transform3f view_matrix;
    view_matrix.matrix() = camera.model().matrix().cast<float>();
    Transform3f proj_matrix;
    proj_matrix.matrix() = camera.projection().matrix().cast<float>();
    m_viewer.render(view_matrix, proj_matrix, layout);

    if (ImGui::Begin("Preview debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        bool visible = m_viewer.is_legend_visible();
        if (ImGui::Checkbox("Show legend", &visible))
            m_viewer.toggle_legend_visible();
        visible = m_viewer.is_gcodewindow_visible();
        if (ImGui::Checkbox("Show gcode window", &visible))
            m_viewer.toggle_gcodewindow_visible();
    }
    ImGui::End();
}

void PreviewRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    m_scene_presenter =
        std::make_unique<Plater::ScenePresenter>(m_workbench, m_project_interactor, *m_device);
    m_project_interactor.add_selected_project_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_changed_listener(m_scene_presenter.get());

    init_viewer(device);
    send_data_to_viewer();
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

    if (is_editor) {
        // legend's custom options
        CustomOption& shells_option = settings.custom_options.emplace_back(CustomOption());
        shells_option.name = _u8L("Shells");
        shells_option.icon = ImGui::LegendShells;
        shells_option.cb_action = std::bind(&PreviewRenderModule::on_legend_shells_action, this, std::placeholders::_1);
    }

    if (m_viewer.init(device, settings))
        m_viewer.set_lights(m_viewer.default_lights());
    else {
        // log some error message
    }
}

//
// Temporary function for test
//
static ProcessorResult import_test_result()
{
    // >>> filename of the test file
    const std::string filename = "c:/temp/processor_result.txt";

    ProcessorResult ret;

    FILE* f = { boost::nowide::fopen(filename.c_str(), "rb") };
    if (f != nullptr) {
        int producer = 0;
        fread(&producer, 1, sizeof(producer), f);
        ret.producer = GCodeProducer(producer);
        fread(&ret.extruders_count, 1, sizeof(ret.extruders_count), f);
        int spiral_vase_enabled = 0;
        fread(&spiral_vase_enabled, 1, sizeof(spiral_vase_enabled), f);
        ret.spiral_vase_enabled = spiral_vase_enabled == 1;
        fread(&ret.z_offset, 1, sizeof(ret.z_offset), f);
        fread(&ret.max_print_height, 1, sizeof(ret.max_print_height), f);
        int filament_diameters = 0;
        fread(&filament_diameters, 1, sizeof(filament_diameters), f);
        for (int i = 0; i < filament_diameters; ++i) {
            float filament_diameter = 0.0f;
            fread(&filament_diameter, 1, sizeof(filament_diameter), f);
            ret.filament_diameters.push_back(filament_diameter);
        }
        int filament_densities = 0;
        fread(&filament_densities, 1, sizeof(filament_densities), f);
        for (int i = 0; i < filament_densities; ++i) {
            float filament_density = 0.0f;
            fread(&filament_density, 1, sizeof(filament_density), f);
            ret.filament_densities.push_back(filament_density);
        }
        int filament_costs = 0;
        fread(&filament_costs, 1, sizeof(filament_costs), f);
        for (int i = 0; i < filament_costs; ++i) {
            float filament_cost = 0.0f;
            fread(&filament_cost, 1, sizeof(filament_cost), f);
            ret.filament_costs.push_back(filament_cost);
        }
        int bed_vertices = 0;
        fread(&bed_vertices, 1, sizeof(bed_vertices), f);
        for (int i = 0; i < bed_vertices; ++i) {
            Vec2f bed_vertex = Vec2f::Zero();
            fread(&bed_vertex.x(), 1, sizeof(bed_vertex.x()), f);
            fread(&bed_vertex.y(), 1, sizeof(bed_vertex.y()), f);
            ret.bed_shape.push_back(bed_vertex);
        }
        int gcode_lines = 0;
        fread(&gcode_lines, 1, sizeof(gcode_lines), f);
        for (int i = 0; i < gcode_lines; ++i) {
            int line_length = 0;
            fread(&line_length, 1, sizeof(line_length), f);
            std::string data(line_length, '\0');
            fread(data.data(), 1, size_t(line_length), f);
            ret.gcode.push_lines(data);
        }
        int extruder_str_colors = 0;
        fread(&extruder_str_colors, 1, sizeof(extruder_str_colors), f);
        for (int i = 0; i < extruder_str_colors; ++i) {
            int line_length = 0;
            fread(&line_length, 1, sizeof(line_length), f);
            std::string data(line_length, '\0');
            fread(data.data(), 1, size_t(line_length), f);
            ret.extruder_str_colors.push_back(data);
        }
        int moves = 0;
        fread(&moves, 1, sizeof(moves), f);
        for (int i = 0; i < moves; ++i) {
            MoveVertex m;
            int type = 0;
            fread(&type, 1, sizeof(type), f);
            m.type = MoveType(type);
            int extrusion_role = 0;
            fread(&extrusion_role, 1, sizeof(extrusion_role), f);
            m.extrusion_role = GCodeExtrusionRole(extrusion_role);
            fread(&m.extruder_id, 1, sizeof(m.extruder_id), f);
            fread(&m.cp_color_id, 1, sizeof(m.cp_color_id), f);
            fread(&m.gcode_id, 1, sizeof(m.gcode_id), f);
            fread(&m.layer_id, 1, sizeof(m.layer_id), f);
            int internal_only = 0;
            fread(&internal_only, 1, sizeof(internal_only), f);
            m.internal_only = internal_only == 1;
            fread(&m.delta_extruder, 1, sizeof(m.delta_extruder), f);
            fread(&m.feedrate, 1, sizeof(m.feedrate), f);
            fread(&m.actual_feedrate, 1, sizeof(m.actual_feedrate), f);
            fread(&m.width, 1, sizeof(m.width), f);
            fread(&m.height, 1, sizeof(m.height), f);
            fread(&m.mm3_per_mm, 1, sizeof(m.mm3_per_mm), f);
            fread(&m.fan_speed, 1, sizeof(m.fan_speed), f);
            fread(&m.temperature, 1, sizeof(m.temperature), f);
            fread(&m.mass, 1, sizeof(m.mass), f);
            fread(&m.position.x(), 1, sizeof(m.position.x()), f);
            fread(&m.position.y(), 1, sizeof(m.position.y()), f);
            fread(&m.position.z(), 1, sizeof(m.position.z()), f);
            int times = 0;
            fread(&times, 1, sizeof(times), f);
            for (int j = 0; j < times; ++j) {
                fread(&m.time[j], 1, sizeof(m.time[j]), f);
            }
            ret.moves.push_back(m);
        }
        int custom_gcode_per_print_z = 0;
        fread(&custom_gcode_per_print_z, 1, sizeof(custom_gcode_per_print_z), f);
        for (int i = 0; i < custom_gcode_per_print_z; ++i) {
            CustomGCode::Item c;
            fread(&c.print_z, 1, sizeof(c.print_z), f);
            int type = 0;
            fread(&type, 1, sizeof(type), f);
            c.type = CustomGCode::Type(type);
            fread(&c.extruder, 1, sizeof(c.extruder), f);
            int line_length = 0;
            fread(&line_length, 1, sizeof(line_length), f);
            c.color = std::string(line_length, '\0');
            fread(c.color.data(), 1, size_t(line_length), f);
            line_length = 0;
            fread(&line_length, 1, sizeof(line_length), f);
            c.extra = std::string(line_length, '\0');
            fread(c.extra.data(), 1, size_t(line_length), f);
        }
        int modes = 0;
        fread(&modes, 1, sizeof(modes), f);
        for (int i = 0; i < modes; ++i) {
            Biz::libpgcode::PrintEstimatedStatistics::Mode& m = ret.print_statistics.modes[i];
            fread(&m.time, 1, sizeof(m.time), f);
            int custom_gcode_times = 0;
            fread(&custom_gcode_times, 1, sizeof(custom_gcode_times), f);
            for (int j = 0; j < custom_gcode_times; ++j) {
                std::pair<Slic3r::CustomGCode::Type, std::pair<float, float>> cgt;
                int type = 0;
                fread(&type, 1, sizeof(type), f);
                cgt.first = Slic3r::CustomGCode::Type(type);
                fread(&cgt.second.first, 1, sizeof(cgt.second.first), f);
                fread(&cgt.second.second, 1, sizeof(cgt.second.second), f);
                m.custom_gcode_times.push_back(cgt);
            }            
        }
        int volumes_per_color_change = 0;
        fread(&volumes_per_color_change, 1, sizeof(volumes_per_color_change), f);
        for (int i = 0; i < volumes_per_color_change; ++i) {
            float volume = 0.0f;
            fread(&volume, 1, sizeof(volume), f);
            ret.print_statistics.volumes_per_color_change.push_back(volume);
        }
        int volumes_per_extruder = 0;
        fread(&volumes_per_extruder, 1, sizeof(volumes_per_extruder), f);
        for (int i = 0; i < volumes_per_extruder; ++i) {
            uint8_t id = 0;
            fread(&id, 1, sizeof(id), f);
            float volume = 0.0f;
            fread(&volume, 1, sizeof(volume), f);
            ret.print_statistics.volumes_per_extruder[id] = volume;
        }
        int cost_per_extruder = 0;
        fread(&cost_per_extruder, 1, sizeof(cost_per_extruder), f);
        for (int i = 0; i < cost_per_extruder; ++i) {
            uint8_t id = 0;
            fread(&id, 1, sizeof(id), f);
            float cost = 0.0f;
            fread(&cost, 1, sizeof(cost), f);
            ret.print_statistics.cost_per_extruder[id] = cost;
        }
        int used_filaments_per_role = 0;
        fread(&used_filaments_per_role, 1, sizeof(used_filaments_per_role), f);
        for (int i = 0; i < used_filaments_per_role; ++i) {
            int role = 0;
            fread(&role, 1, sizeof(role), f);
            std::pair<float, float> values;
            fread(&values.first, 1, sizeof(values.first), f);
            fread(&values.second, 1, sizeof(values.second), f);
            ret.print_statistics.used_filaments_per_role[Slic3r::GCodeExtrusionRole(role)] = values;
        }
        int line_length = 0;
        fread(&line_length, 1, sizeof(line_length), f);
        ret.print_settings.print = std::string(line_length, '\0');
        fread(ret.print_settings.print.data(), 1, size_t(line_length), f);
        line_length = 0;
        fread(&line_length, 1, sizeof(line_length), f);
        ret.print_settings.printer = std::string(line_length, '\0');
        fread(ret.print_settings.printer.data(), 1, size_t(line_length), f);
        int filaments = 0;
        fread(&filaments, 1, sizeof(filaments), f);
        for (int i = 0; i < filaments; ++i) {
            line_length = 0;
            fread(&line_length, 1, sizeof(line_length), f);
            std::string data(line_length, '\0');
            fread(data.data(), 1, size_t(line_length), f);
            ret.print_settings.filament.push_back(data);
        }

        // MISSING gcode_result.conflict_result -> it contains pointers

        fclose(f);
    }
    return ret;
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

    for (size_t i = 0; i < result.gcode.size(); ++i) {
        ret.gcode.push_back(std::string(result.gcode[i]));
    }

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

void PreviewRenderModule::send_data_to_viewer()
{
    ProcessorResult result = import_test_result();
    ViewerInputData viewer_data = extract_viewer_input_data_from_result(std::move(result));
    LibvgcodeWrapper::WrapperInputData wrapper_data = extract_wrapper_input_data_from_result(result);

    m_viewer.reset_default_extrusion_roles_colors();
    m_viewer.load(std::move(wrapper_data), std::move(viewer_data));
    if (!m_viewer.has_data()) {
        // log some error message
        return;
    }
}

void PreviewRenderModule::on_invalidate_slice()
{
    // TODO -> implement me
}

void PreviewRenderModule::on_update_layers_slider(const Slic3r::CustomGCode::Info& info)
{
    // TODO -> implement me
}

void PreviewRenderModule::on_request_extra_frames(unsigned int count)
{
    // TODO -> implement me
}

void PreviewRenderModule::on_gcode_view_type_changed()
{
    // TODO -> implement me
}

void PreviewRenderModule::on_slider_layers_on_thumb_move()
{
    // TODO -> implement me
}

void PreviewRenderModule::on_slider_layers_ticks_changed()
{
    // TODO -> implement me
}

std::vector<std::string> PreviewRenderModule::on_slider_layers_get_extruder_colors()
{
    // TODO -> implement me
    return std::vector<std::string>();
}

bool PreviewRenderModule::on_slider_layers_auto_color_change()
{
    // TODO -> implement me
    return false;
}

void PreviewRenderModule::on_slider_layers_check_gcode(Slic3r::CustomGCode::Type type)
{
    // TODO -> implement me
}

bool PreviewRenderModule::on_slider_layers_get_extruders_sequence(LibvgcodeWrapper::ExtrudersSequence& sequence)
{
    // TODO -> implement me
    return false;
}

std::string PreviewRenderModule::on_slider_layers_get_custom_code(const std::string& code_in, float height)
{
    // TODO -> implement me
    return "";
}

std::string PreviewRenderModule::on_slider_layers_get_pause_print_msg(const std::string& msg_in, float height)
{
    // TODO -> implement me
    return "";
}

std::string PreviewRenderModule::on_slider_layers_get_new_color(const std::string& color)
{
    // TODO -> implement me
    return "";
}

int PreviewRenderModule::on_slider_layers_show_info_msg(const std::string& message, int btns_flag)
{
    // TODO -> implement me
    return 0;
}

std::string PreviewRenderModule::on_slider_layers_get_gcode(Slic3r::CustomGCode::Type type)
{
    // TODO -> implement me
    return "";
}

std::set<int> PreviewRenderModule::on_slider_layers_get_used_extruders_in_print(float print_z)
{
    // TODO -> implement me
    return std::set<int>();
}

void PreviewRenderModule::on_slider_layers_app_config_changed(const std::string& key, const std::string& val)
{
    // TODO -> implement me
}

void PreviewRenderModule::on_legend_shells_action(bool visible)
{
    // TODO -> implement me
}

} // namespace Slic3r::App::Preview
