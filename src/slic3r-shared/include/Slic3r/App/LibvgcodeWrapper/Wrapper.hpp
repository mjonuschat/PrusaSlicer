#pragma once

#include "Slic3r/App/LibvgcodeWrapper/Types.hpp"
#include "Slic3r/App/Imgui/DoubleSlider.hpp"
#include "libslic3r/BoundingBox.hpp"

#include <Slic3r/App/libvgcode/ViewerInputData.hpp>

#include <memory>

namespace Slic3r::App::Render {
class Device;
class ImguiRender;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {
class Scene;
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::LibvgcodeWrapper {

enum class WrapperMode
{
    EditorGCode,
    GCodeViewer,
    EditorPreGCode,
    EditorSLA,
};

struct WrapperSettings
{
    WrapperMode mode{ WrapperMode::EditorGCode };
    bool slider_layers_editable{ false };
    bool slider_layers_show_ruler{ false };
    bool slider_layers_show_ruler_bg{ false };
    bool slider_layers_show_estimated_times{ false };
    bool slider_layers_use_default_colors{ false };
    bool seq_top_layer_only{ false };
    bool gcodewindow_visible{ true };

    libvgcode::CustomOptions custom_options;

    //
    // wrapper callbacks
    //
    InvalidateSliceCallback                         cb_invalidate_slice{ nullptr };
    Imgui::DoubleSlider::RequestExtraFramesCallback cb_request_extra_frames{ nullptr };
    UpdateLayersSlider                              cb_update_layers_slider{ nullptr };
    GCodeViewTypeChangedCallback                    cb_gcode_view_type_changed{ nullptr };

    //
    // layers slider callbacks
    //
    Imgui::DoubleSlider::OnThumbMoveCallback        cb_slider_layers_on_thumb_move{ nullptr };
    TicksChangedCallback                            cb_slider_layers_ticks_changed{ nullptr };
    AutoColorChangeCallback                         cb_slider_layers_auto_color_change{ nullptr };
    NotifyEmptyAutoColorChangeCallback              cb_slider_layers_notify_empty_auto_color_change{ nullptr };
    NotifyEmptyColorChangeGCodeCallback             cb_slider_layers_notify_empty_color_change_gcode{ nullptr };
    GetExtrudersSequenceCallback                    cb_slider_layers_get_extruders_sequence{ nullptr };
    ShowInfoMsgCallback                             cb_slider_layers_show_info_msg{ nullptr };
    GetUsedExtrudersInPrintCallback                 cb_slider_layers_get_used_extruders_in_print{ nullptr };
    AppConfigChangedCallback                        cb_slider_layers_app_config_changed{ nullptr };
    //
    // gcode slider callbacks
    //
    Imgui::DoubleSlider::OnThumbMoveCallback        cb_slider_gcode_on_thumb_move{ nullptr };
};

class WrapperImpl;
struct WrapperInputData;
struct WrapperSLAInputData;

struct WrapperLayoutData
{
    float menubar_height{ 0.0f };
    std::array<float, 2> view_toolbar_size{ 0.0f, 0.0f };
    float collapse_toolbar_height{ 0.0f };
    float scale_factor{ 1.0f };
};

class Wrapper
{
public:
    Wrapper();
    Wrapper(Wrapper&&) = delete;
    Wrapper(const Wrapper&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;
    Wrapper& operator=(const Wrapper&) = delete;
    ~Wrapper();

    bool init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory,
        const WrapperSettings& settings);

    WrapperMode mode() const;
    void set_mode(WrapperMode mode);

    void reset();

    void load(WrapperInputData&& wrapper_data, libvgcode::ViewerInputData&& data);
    void load_as_sla(WrapperSLAInputData&& wrapper_sla_data);

    libvgcode::ViewType view_type() const;
    void set_view_type(libvgcode::ViewType type);

    BoundingBoxf3 bounding_box(const Biz::libpgcode::MoveTypes& types = {
        Biz::libpgcode::MoveType::Retract,
        Biz::libpgcode::MoveType::Unretract,
        Biz::libpgcode::MoveType::Seam,
        Biz::libpgcode::MoveType::ToolChange,
        Biz::libpgcode::MoveType::ColorChange,
        Biz::libpgcode::MoveType::PausePrint,
        Biz::libpgcode::MoveType::CustomGCode,
        Biz::libpgcode::MoveType::Travel,
        Biz::libpgcode::MoveType::Wipe,
        Biz::libpgcode::MoveType::Extrude }) const;

    void render_toolpaths(const Vec3f& camera_position);
    void render_gui(const WrapperLayoutData& layout);
    void render_gcode_window();
    void render_legend(Render::ImguiRender* imgui_render);
    void render_gcode_slider();
    void render_layers_slider();

    Biz::libpgcode::UnitsSystem units() const;
    void set_units(Biz::libpgcode::UnitsSystem sys);

    bool has_data() const;

    void set_legend_visible(bool visible);
    void toggle_legend_visible();
    bool is_legend_visible() const;
    bool is_legend_shown() const;

    void set_gcodewindow_visible(bool visible);
    void toggle_gcodewindow_visible();
    bool is_gcodewindow_visible() const;

    bool is_top_layer_only_view_range() const;
    void toggle_top_layer_only_view_range();

    const libvgcode::Interval& view_visible_range() const;
    const libvgcode::Interval& view_enabled_range() const;

    bool is_option_visible(Biz::libpgcode::OptionType type);
    void toggle_option_visibility(Biz::libpgcode::OptionType type);
    const Biz::libpgcode::OptionTypes& options() const;

    const libvgcode::Interval& layers_range() const;
    void set_layers_range(libvgcode::Interval::value_type min, libvgcode::Interval::value_type max);

    void set_extrusion_role_color(Domain::GCodeExtrusionRole role, const ColorRGB& color);

    const libvgcode::GCodeEvents& gcode_events() const;
    uint8_t used_extruders_count() const;
    std::vector<uint8_t> used_extruders_ids() const;

    void slider_gcode_move_current_thumb(int delta);
    void slider_layers_move_current_thumb(int delta);
    void slider_layers_jump_to_value();
    void slider_layers_add_current_tick();
    void slider_layers_delete_current_tick();

    const libvgcode::Lights& lights() const;
    void set_lights(const libvgcode::Lights& lights);
    const libvgcode::Lights& default_lights() const;

    float cog_marker_scale_factor() const;
    void set_cog_marker_scale_factor(float factor);

    bool tool_marker_enabled() const;
    void set_tool_marker_enabled(bool enabled);
    float tool_marker_scale_factor() const;
    void set_tool_marker_scale_factor(float factor);

    void set_extrusion_roles_colors_popup_visible(bool show);
    void set_options_colors_popup_visible(bool show);
    void set_scale_factor_popup_type(Biz::libpgcode::OptionType type);
    void set_range_colors_popup_type(libvgcode::ViewType type);
    void set_radius_popup_type(Biz::libpgcode::MoveType type);

    void reset_default_extrusion_roles_colors();

private:
    std::unique_ptr<WrapperImpl> m_impl;
};

} // namespace Slic3r::App::LibvgcodeWrapper
