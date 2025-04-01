#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperImpl.hpp"

using namespace Slic3r::App::libvgcode;
using namespace Slic3r::Biz::libpgcode;

namespace Slic3r::App::LibvgcodeWrapper {

Wrapper::Wrapper()
    : m_impl(new WrapperImpl)
{
}

Wrapper::~Wrapper() = default;

bool Wrapper::init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory,
    const WrapperSettings& settings)
{
    return m_impl->init(device, scene, data_factory, settings);
}

WrapperMode Wrapper::mode() const
{
    return m_impl->mode();
}

void Wrapper::set_mode(WrapperMode mode)
{
    m_impl->set_mode(mode);
}

void Wrapper::reset()
{
    m_impl->reset();
}

void Wrapper::load(WrapperInputData&& wrapper_data, ViewerInputData&& data)
{
    m_impl->load(std::move(wrapper_data), std::move(data));
}

void Wrapper::load_as_sla(WrapperSLAInputData&& wrapper_sla_data)
{
    m_impl->load_as_sla(std::move(wrapper_sla_data));
}

void Wrapper::set_extrusion_role_color(Domain::GCodeExtrusionRole role, const ColorRGB& color)
{
    m_impl->set_extrusion_role_color(role, color);
}

ViewType Wrapper::view_type() const
{
    return m_impl->view_type();
}

void Wrapper::set_view_type(ViewType type)
{
    m_impl->set_view_type(type);
}

BoundingBoxf3 Wrapper::bounding_box(const Biz::libpgcode::MoveTypes& types) const
{
    return m_impl->bounding_box(types);
}

void Wrapper::render_toolpaths(const Vec3f& camera_position)
{
    m_impl->render_toolpaths(camera_position);
}

void Wrapper::render_gui(const WrapperLayoutData& layout)
{
    m_impl->render_gui(layout);
}

void Wrapper::render_gcode_window()
{
    m_impl->render_gcode_window();
}

void Wrapper::render_legend(Render::ImguiRender* imgui_render)
{
    m_impl->render_legend(imgui_render);
}

void Wrapper::render_gcode_slider()
{
    m_impl->render_gcode_slider();
}

void Wrapper::render_layers_slider()
{
    m_impl->render_layers_slider();
}

UnitsSystem Wrapper::units() const
{
    return m_impl->units();
}

void Wrapper::set_units(UnitsSystem sys)
{
    m_impl->set_units(sys);
}

bool Wrapper::has_data() const
{
    return m_impl->has_data();
}

void Wrapper::set_legend_visible(bool visible)
{
    m_impl->set_legend_visible(visible);
}

void Wrapper::toggle_legend_visible()
{
    m_impl->toggle_legend_visible();
}

bool Wrapper::is_legend_visible() const
{
    return m_impl->is_legend_visible();
}

bool Wrapper::is_legend_shown() const
{
    return m_impl->is_legend_shown();
}

void Wrapper::set_gcodewindow_visible(bool visible)
{
    m_impl->set_gcodewindow_visible(visible);
}

void Wrapper::toggle_gcodewindow_visible()
{
    m_impl->toggle_gcodewindow_visible();
}

bool Wrapper::is_gcodewindow_visible() const
{
    return m_impl->is_gcodewindow_visible();
}

bool Wrapper::is_top_layer_only_view_range() const
{
    return m_impl->is_top_layer_only_view_range();
}

void Wrapper::toggle_top_layer_only_view_range()
{
    m_impl->toggle_top_layer_only_view_range();
}

const Interval& Wrapper::view_visible_range() const
{
    return m_impl->view_visible_range();
}

const Interval& Wrapper::view_enabled_range() const
{
    return m_impl->view_enabled_range();
}

bool Wrapper::is_option_visible(Biz::libpgcode::OptionType type)
{
    return m_impl->is_option_visible(type);
}

void Wrapper::toggle_option_visibility(Biz::libpgcode::OptionType type)
{
    m_impl->toggle_option_visibility(type);
}

const Biz::libpgcode::OptionTypes& Wrapper::options() const
{
    return m_impl->options();
}

const Interval& Wrapper::layers_range() const
{
    return m_impl->layers_range();
}

void Wrapper::set_layers_range(Interval::value_type min, Interval::value_type max)
{
    m_impl->set_layers_range(min, max);
}

const GCodeEvents& Wrapper::gcode_events() const
{
    return m_impl->gcode_events();
}

uint8_t Wrapper::used_extruders_count() const
{
    return m_impl->used_extruders_count();
}

std::vector<uint8_t> Wrapper::used_extruders_ids() const
{
    return m_impl->used_extruders_ids();
}

void Wrapper::slider_gcode_move_current_thumb(int delta)
{
    m_impl->slider_gcode_move_current_thumb(delta);
}

void Wrapper::slider_layers_move_current_thumb(int delta)
{
    m_impl->slider_layers_move_current_thumb(delta);
}

void Wrapper::slider_layers_jump_to_value()
{
    m_impl->slider_layers_jump_to_value();
}

void Wrapper::slider_layers_add_current_tick()
{
    m_impl->slider_layers_add_current_tick();
}

void Wrapper::slider_layers_delete_current_tick()
{
    m_impl->slider_layers_delete_current_tick();
}

const Lights& Wrapper::lights() const
{
    return m_impl->lights();
}

void Wrapper::set_lights(const Lights& lights)
{
    m_impl->set_lights(lights);
}

const Lights& Wrapper::default_lights() const
{
    return m_impl->default_lights();
}

float Wrapper::cog_marker_scale_factor() const
{
    return m_impl->cog_marker_scale_factor();
}

void Wrapper::set_cog_marker_scale_factor(float factor)
{
    m_impl->set_cog_marker_scale_factor(factor);
}

bool Wrapper::tool_marker_enabled() const
{
    return m_impl->tool_marker_enabled();
}

void Wrapper::set_tool_marker_enabled(bool enabled)
{
    m_impl->set_tool_marker_enabled(enabled);
}

float Wrapper::tool_marker_scale_factor() const
{
    return m_impl->tool_marker_scale_factor();
}

void Wrapper::set_tool_marker_scale_factor(float factor)
{
    m_impl->set_tool_marker_scale_factor(factor);
}

void Wrapper::set_extrusion_roles_colors_popup_visible(bool show)
{
    m_impl->set_extrusion_roles_colors_popup_visible(show);
}

void Wrapper::set_options_colors_popup_visible(bool show)
{
    m_impl->set_options_colors_popup_visible(show);
}

void Wrapper::set_scale_factor_popup_type(Biz::libpgcode::OptionType type)
{
    m_impl->set_scale_factor_popup_type(type);
}

void Wrapper::set_range_colors_popup_type(libvgcode::ViewType type)
{
    m_impl->set_range_colors_popup_type(type);
}

void Wrapper::set_radius_popup_type(Biz::libpgcode::MoveType type)
{
    m_impl->set_radius_popup_type(type);
}

void Wrapper::reset_default_extrusion_roles_colors()
{
    m_impl->reset_default_extrusion_roles_colors();
}

} // namespace Slic3r::App::LibvgcodeWrapper
