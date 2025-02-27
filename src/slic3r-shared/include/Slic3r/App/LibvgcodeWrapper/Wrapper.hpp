#pragma once

#include "Slic3r/App/LibvgcodeWrapper/Types.hpp"
#include "Slic3r/App/Imgui/DoubleSlider.hpp"

#include <memory>

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {
class Scene;
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::libvgcode {
struct ViewerInputData;
} // namespace Slic3r::App::libvgcode;

namespace Slic3r::App::LibvgcodeWrapper {

struct WrapperSettings
{
    bool slider_layers_editable{ false };
    bool slider_layers_show_ruler{ false };
    bool slider_layers_show_ruler_bg{ false };
    bool slider_layers_show_estimated_times{ false };
    bool settings_in_legend_visible{ false };
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
    GetExtruderColorsCallback                       cb_slider_layers_get_extruder_colors{ nullptr };
    AutoColorChangeCallback                         cb_slider_layers_auto_color_change{ nullptr };
    CheckGCodeCallback                              cb_slider_layers_check_gcode{ nullptr };
    GetExtrudersSequenceCallback                    cb_slider_layers_get_extruders_sequence{ nullptr };
    GetCustomGCodeCallback                          cb_slider_layers_get_custom_code{ nullptr };
    GetPausePrintMsgCallback                        cb_slider_layers_get_pause_print_msg{ nullptr };
    GetNewColorCallback                             cb_slider_layers_get_new_color{ nullptr };
    ShowInfoMsgCallback                             cb_slider_layers_show_info_msg{ nullptr };
    GetGCodeCallback                                cb_slider_layers_get_gcode{ nullptr };
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
    void shutdown();

    void reset();

    void load(WrapperInputData&& wrapper_data, libvgcode::ViewerInputData&& data);
    void load_as_sla(WrapperSLAInputData&& wrapper_sla_data);

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

    Biz::libpgcode::UnitsSystem units() const;
    void set_units(Biz::libpgcode::UnitsSystem sys);

    bool has_data() const;

    void set_legend_visible(bool visible);
    void toggle_legend_visible();
    bool is_legend_visible() const;

    void set_gcodewindow_visible(bool visible);
    void toggle_gcodewindow_visible();
    bool is_gcodewindow_visible() const;

    const libvgcode::Lights& lights() const;
    void set_lights(const libvgcode::Lights& lights);
    const libvgcode::Lights& default_lights() const;

    float cog_marker_scale_factor() const;
    void set_cog_marker_scale_factor(float factor);

    float tool_marker_scale_factor() const;
    void set_tool_marker_scale_factor(float factor);

    void set_scale_factor_popup_type(Biz::libpgcode::OptionType type);

    void reset_default_extrusion_roles_colors();

private:
    std::unique_ptr<WrapperImpl> m_impl;
};

} // namespace Slic3r::App::LibvgcodeWrapper
