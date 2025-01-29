#pragma once

#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperInputData.hpp"
#include "Slic3r/App/LibvgcodeWrapper/DoubleSliderForGCode.hpp"
#include "Slic3r/App/LibvgcodeWrapper/DoubleSliderForLayers.hpp"
#include "Slic3r/App/LibvgcodeWrapper/GCodeWindow.hpp"
#include "Slic3r/App/LibvgcodeWrapper/ActualSpeedPlotter.hpp"
#include "Slic3r/App/LibvgcodeWrapper/Legend.hpp"
#include "Slic3r/Biz/Units.hpp"

#include <Slic3r/App/libvgcode/Viewer.hpp>

namespace Slic3r::App::LibvgcodeWrapper {

struct LegendParams
{
    bool visible{ true };
    bool enabled{ true };
    bool settings_visible{ false };
    bool is_shown() const { return enabled && visible; }
};

class WrapperImpl
{
public:
    WrapperImpl() = default;
    WrapperImpl(WrapperImpl&&) = delete;
    WrapperImpl(const WrapperImpl&) = delete;
    WrapperImpl& operator=(WrapperImpl&&) = delete;
    WrapperImpl& operator=(const WrapperImpl&) = delete;
    ~WrapperImpl() = default;

    bool init(App::Render::Device& device, const WrapperSettings& settings);
    void shutdown();

    void reset();

    void load(WrapperInputData&& wrapper_data, libvgcode::ViewerInputData&& data);
    void load_as_sla(WrapperSLAInputData&& wrapper_sla_data);

    void render(const Transform3f& view_matrix, const Transform3f& projection_matrix,
        const WrapperLayoutData& layout);

    Biz::libpgcode::UnitsSystem units() const { return m_units; }
    void set_units(Biz::libpgcode::UnitsSystem sys);

    Biz::libpgcode::GCodeProducer producer() const { return m_data.producer; }

    const libvgcode::CustomOptions& custom_options() const { return m_settings.custom_options; }
    libvgcode::CustomOptions& custom_options() { return m_settings.custom_options; }

    bool has_data() const { return m_viewer.layers_count() > 0; }

    const libvgcode::Lights& lights() const { return m_viewer.lights(); }
    void set_lights(const libvgcode::Lights& lights) { m_viewer.set_lights(lights); }
    const libvgcode::Lights& default_lights() const { return m_viewer.default_lights(); }

    void set_legend_visible(bool visible) { m_legend_params.visible = visible; }
    void toggle_legend_visible() { set_legend_visible(!m_legend_params.visible); }
    bool is_legend_visible() const { return m_legend_params.visible; }

    void set_legend_enabled(bool enabled) { m_legend_params.enabled = enabled; }
    void toggle_legend_enabled() { set_legend_enabled(!m_legend_params.enabled); }
    bool is_legend_enabled() const { return m_legend_params.enabled; }

    void set_gcodewindow_visible(bool visible) { m_gcode_window_data.set_visible(visible); }
    void toggle_gcodewindow_visible() { m_gcode_window_data.toggle_visible(); }
    bool is_gcodewindow_visible() const { return m_gcode_window_data.is_visible(); }

    void set_settings_in_legend_visible(bool visible) { m_legend_params.settings_visible = visible; }

    bool is_legend_shown() const { return m_legend_params.is_shown(); }

    void set_layers_range(libvgcode::Interval::value_type min, libvgcode::Interval::value_type max);

    void reset_default_extrusion_roles_colors() { m_viewer.reset_default_extrusion_roles_colors(); }

    void set_extrusion_roles_colors_popup_visible(bool show) { m_extrusion_roles_colors_popup_visible = show; }
    void toggle_extrusion_roles_colors_popup_visible() { m_extrusion_roles_colors_popup_visible = !m_extrusion_roles_colors_popup_visible; }
    bool is_extrusion_roles_colors_popup_visible() const { return m_extrusion_roles_colors_popup_visible; }

    void set_options_colors_popup_visible(bool show) { m_options_colors_popup_visible = show; }
    void toggle_options_colors_popup_visible() { m_options_colors_popup_visible = !m_options_colors_popup_visible; }
    bool is_options_colors_popup_visible() const { return m_options_colors_popup_visible; }

    void set_range_colors_popup_type(libvgcode::ViewType type);
    void set_radius_popup_type(Biz::libpgcode::MoveType type);
    void set_scale_factor_popup_type(Biz::libpgcode::OptionType type);

private:
    WrapperSettings m_settings;
    WrapperInputData m_data;
    PrinterTechnology m_printer_technology{ PrinterTechnology::FFF };

    libvgcode::Viewer m_viewer;
    DoubleSliderForGcode m_slider_gcode;
    DoubleSliderForLayers m_slider_layers;
    GCodeWindowData m_gcode_window_data;
    ActualSpeedPlotData m_actual_speed_plot_data;
    LegendParams m_legend_params;
    LegendCallbacks m_cb_legend;

    float m_legend_height{ 0.0f };
    Biz::libpgcode::UnitsSystem m_units{ Biz::libpgcode::UnitsSystem::SI };

    bool m_extrusion_roles_colors_popup_visible{ false };
    bool m_options_colors_popup_visible{ false };
    libvgcode::ViewType m_range_colors_popup_type{ libvgcode::ViewType::COUNT };
    Biz::libpgcode::MoveType m_radius_popup_type{ Biz::libpgcode::MoveType::COUNT };
    Biz::libpgcode::OptionType m_scale_factor_popup_type{ Biz::libpgcode::OptionType::COUNT };

private:
    void update_slider_gcode(std::optional<size_t> visible_range_min = std::nullopt,
                             std::optional<size_t> visible_range_max = std::nullopt);
    void update_slider_layers();
    void update_view_visible_range(size_t first, size_t last);

    void on_slider_layers_scroll_changed();
    void on_slider_layers_check_gcode(Slic3r::CustomGCode::Type type);
    void on_slider_gcode_scroll_changed();
    void on_extrusion_role_visibility_changed();
    void on_request_extra_frames(unsigned int count = 1);

    void render_legend(const WrapperLayoutData& layout);
    void render_slider_gcode(const WrapperLayoutData& layout);
    void render_slider_layers(const WrapperLayoutData& layout);
    void render_gcodewindow(const WrapperLayoutData& layout);
    void render_vertex_properties(const WrapperLayoutData& layout);

    void render_customize_extrusion_roles_colors_popup();
    void render_customize_options_colors_popup();
    void render_customize_range_colors_popup();
    void render_customize_radius_popup();
    void render_customize_scale_factor_popup();
};

} // namespace Slic3r::App::LibvgcodeWrapper
