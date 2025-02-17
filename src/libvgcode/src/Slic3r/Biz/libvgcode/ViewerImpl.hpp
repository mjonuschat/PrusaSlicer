///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Settings.hpp"
#include "SegmentTemplate.hpp"
#include "OptionTemplate.hpp"
#include "CogMarker.hpp"
#include "ToolMarker.hpp"
#include "Bitset.hpp"
#include "ViewRange.hpp"
#include "Layers.hpp"
#include "ExtrusionRoles.hpp"
#include "Extruders.hpp"
#include "Slic3r/Biz/libvgcode/ColorRange.hpp"
#include "Slic3r/Biz/libvgcode/ViewerInputData.hpp"
#include "Slic3r/Biz/libvgcode/Viewer.hpp"

#include <Slic3r/Biz/libpgcode/ProcessorResult.hpp>

#include <Slic3r/App/Render/Buffer.hpp>

#define USE_TEXTURE_BUFFER (0 && (!SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)))

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Biz::libvgcode {

struct GCodeInputData;

class ViewerImpl
{
public:
    ViewerImpl();
    ~ViewerImpl() { shutdown(); }
    ViewerImpl(const ViewerImpl& other) = delete;
    ViewerImpl(ViewerImpl&& other) = delete;
    ViewerImpl& operator = (const ViewerImpl& other) = delete;
    ViewerImpl& operator = (ViewerImpl&& other) = delete;

    //
    // Initialize shaders, uniform indices and segment geometry.
    //
    void init(App::Render::Device& device);
    //
    // Release the resources used by the viewer.
    //
    void shutdown();
    //
    // Reset all caches and free gpu memory.
    //
    void reset();
    //
    // Setup all the variables used for visualization of the toolpaths
    // from the given gcode data.
    //
    void load(ViewerInputData&& gcode_data);
    //
    // Setup the viewer content from the given data (support for SLA printers).
    //
    void load_as_sla(const std::vector<float>& layers_zs, const std::vector<float>& layers_times);

    //
    // Update the visibility property of toolpaths in dependence
    // of the current settings
    //
    void update_enabled_entities();
    //
    // Update the color of toolpaths in dependence of the current
    // view type and settings
    //
    void update_colors();
    void update_colors_texture();

    //
    // Render the toolpaths
    //
    void render(const Transform3f& view_matrix, const Transform3f& projection_matrix);

#if ENABLE_RENDER_TO_TEXTURE 
    std::vector<uint8_t> render_to_texture(uint16_t width, uint16_t height, const Transform3f& view_matrix,
        const Transform3f& projection_matrix, const ColorRGBA& background_color);
#endif // ENABLE_RENDER_TO_TEXTURE

    ViewType view_type() const { return m_settings.view_type; }
    void set_view_type(ViewType type);

    libpgcode::TimeMode time_mode() const { return m_settings.time_mode; }
    void set_time_mode(libpgcode::TimeMode mode);

    const Interval& layers_range() const { return m_layers.view_range(); }
    void set_layers_range(const Interval& range) { set_layers_range(range[0], range[1]); }
    void set_layers_range(Interval::value_type min, Interval::value_type max);

    bool is_top_layer_only_view_range() const { return m_settings.top_layer_only_view_range; }
    void toggle_top_layer_only_view_range();

    bool is_spiral_vase_enabled() const { return m_settings.spiral_vase_enabled; }

    const libpgcode::TimeModes& time_modes() const { return m_time_modes; }

    size_t layers_count() const { return m_layers.count(); }
    float layer_z(size_t layer_id) const { return m_layers.layer_z(layer_id); }
    std::vector<float> layers_zs() const { return m_layers.zs(); }

    size_t layer_id_at(float z) const { return m_layers.layer_id_at(z); }

    uint8_t used_extruders_count() const { return m_used_extruders.extruders_count(); }
    std::vector<uint8_t> used_extruders_ids() const { return m_used_extruders.extruders_ids(); }
    size_t used_extruder_color_prints_count(uint8_t extruder_id) const { return m_used_extruders.extruder_color_prints_count(extruder_id); }
    ColorPrints used_extruder_color_prints(uint8_t extruder_id) const { return m_used_extruders.extruder_color_prints(extruder_id); }
    float used_extruder_used_filament_length(uint8_t extruder_id) const { return m_used_extruders.extruder_used_filament_length(extruder_id); }
    float used_extruder_used_filament_mass(uint8_t extruder_id) const { return m_used_extruders.extruder_used_filament_mass(extruder_id); }

    uint8_t extruders_count() const { return m_extruders_count; }

    BoundingBoxf3 bounding_box(const libpgcode::MoveTypes& types = {
        libpgcode::MoveType::Retract,
        libpgcode::MoveType::Unretract,
        libpgcode::MoveType::Seam,
        libpgcode::MoveType::ToolChange,
        libpgcode::MoveType::ColorChange,
        libpgcode::MoveType::PausePrint,
        libpgcode::MoveType::CustomGCode,
        libpgcode::MoveType::Travel,
        libpgcode::MoveType::Wipe,
        libpgcode::MoveType::Extrude }) const;
    BoundingBoxf3 extrusion_bounding_box(const libpgcode::GCodeExtrusionRoles& roles = {
        GCodeExtrusionRole::Perimeter,
        GCodeExtrusionRole::ExternalPerimeter,
        GCodeExtrusionRole::OverhangPerimeter,
        GCodeExtrusionRole::InternalInfill,
        GCodeExtrusionRole::SolidInfill,
        GCodeExtrusionRole::TopSolidInfill,
        GCodeExtrusionRole::Ironing,
        GCodeExtrusionRole::BridgeInfill,
        GCodeExtrusionRole::GapFill,
        GCodeExtrusionRole::Skirt,
        GCodeExtrusionRole::SupportMaterial,
        GCodeExtrusionRole::SupportMaterialInterface,
        GCodeExtrusionRole::WipeTower,
        GCodeExtrusionRole::Custom }) const;

    bool is_option_visible(libpgcode::OptionType type) const;
    void toggle_option_visibility(libpgcode::OptionType type);

    bool is_extrusion_role_visible(GCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(GCodeExtrusionRole role);

    const Interval& view_full_range() const { return m_view_range.full(); }
    const Interval& view_enabled_range() const { return m_view_range.enabled(); }
    const Interval& view_visible_range() const { return m_view_range.visible(); }
    void set_view_visible_range(Interval::value_type min, Interval::value_type max);

    const Lights& lights() const { return m_lights; }
    void set_lights(const Lights& lights);
    const Lights& default_lights() const;

    size_t vertices_count() const { return m_vertices.size(); }
    const libpgcode::MoveVertices& vertices() const { return m_vertices; }
    const libpgcode::MoveVertex& current_vertex() const { return vertex_at(current_vertex_id()); }
    size_t current_vertex_id() const { return size_t(m_view_range.visible()[1]); }
    const libpgcode::MoveVertex& vertex_at(size_t id) const {
        return (id < m_vertices.size()) ? m_vertices[id] : libpgcode::DUMMY_MOVE_VERTEX;
    }
    float estimated_time() const { return m_total_time[size_t(m_settings.time_mode)]; }
    float estimated_time_at(size_t id) const;
    ColorRGB vertex_color(const libpgcode::MoveVertex& vertex) const;

    size_t extrusion_roles_count() const { return m_extrusion_roles.roles_count(); }
    libpgcode::GCodeExtrusionRoles extrusion_roles() const { return m_extrusion_roles.roles(); }
    size_t visible_extrusion_roles_count() const;
    libpgcode::GCodeExtrusionRoles visible_extrusion_roles() const;
    float extrusion_role_estimated_time(GCodeExtrusionRole role) const {
        return m_extrusion_roles.time(role, m_settings.time_mode);
    }
    float extrusion_role_used_filament_length(GCodeExtrusionRole role) const {
        return m_extrusion_roles.used_filament_length(role);
    }
    float extrusion_role_used_filament_mass(GCodeExtrusionRole role) const {
        return m_extrusion_roles.used_filament_mass(role);
    }

    size_t options_count() const { return m_options.size(); }
    const libpgcode::OptionTypes& options() const { return m_options; }
    size_t visible_options_count() const;
    libpgcode::OptionTypes visible_options() const;
    float option_estimated_time(libpgcode::OptionType type) const;

    std::vector<float> layers_estimated_times() const { return m_layers.times(m_settings.time_mode); }

    size_t gcode_events_count() const { return m_gcode_events.size(); }
    const GCodeEvents& gcode_events() const { return m_gcode_events; }

    size_t tool_colors_count() const { return m_tool_colors.size(); }
    const Palette& tool_colors() const { return m_tool_colors; }
    void set_tool_colors(const Palette& colors);

    size_t color_print_colors_count() const { return m_color_print_colors.size(); }
    const Palette& color_print_colors() const { return m_color_print_colors; }
    void set_color_print_colors(const Palette& colors);

    const ColorRGB& extrusion_role_color(GCodeExtrusionRole role) const;
    void set_extrusion_role_color(GCodeExtrusionRole role, const ColorRGB& color);
    void reset_default_extrusion_roles_colors();

    const ColorRGB& option_color(libpgcode::OptionType type) const;
    void set_option_color(libpgcode::OptionType type, const ColorRGB& color);
    void reset_default_options_colors();

    const ColorRange& color_range(ViewType type) const;
    void set_color_range_palette(ViewType type, const Palette& palette);

    float travels_radius() const { return m_travels_radius; }
    void set_travels_radius(float radius);

    float wipes_radius() const { return m_wipes_radius; }
    void set_wipes_radius(float radius);

    Vec3f cog_marker_position() const { return m_cog_marker.position(); }
    float cog_marker_scale_factor() const { return m_cog_marker_scale_factor; }
    void set_cog_marker_scale_factor(float factor) { m_cog_marker_scale_factor = std::max(factor, 0.001f); }

    float tool_marker_offset_z() const { return m_tool_marker.offset_z(); }
    void set_tool_marker_offset_z(float offset_z) { m_tool_marker.set_offset_z(offset_z); }
    float tool_marker_scale_factor() const { return m_tool_marker_scale_factor; }
    void set_tool_marker_scale_factor(float factor) { m_tool_marker_scale_factor = std::max(factor, 0.001f); }
    const ColorRGB& tool_marker_color() const { return m_tool_marker.color(); }
    void set_tool_marker_color(const ColorRGB& color) { m_tool_marker.set_color(color); }
    float tool_marker_alpha() const { return m_tool_marker.alpha(); }
    void set_tool_marker_alpha(float alpha) { m_tool_marker.set_alpha(alpha); }
    BoundingBoxf3 tool_marker_bounding_box() const;

    bool export_toolpaths_to_obj(FILE& obj_file, FILE& mtl_file, const ObjExportParams& params) const;

private:
    //
    // Settings used to render the toolpaths
    //
    Settings m_settings;
    //
    // Detected layers
    //
    Layers m_layers;
    //
    // Detected extrusion roles
    //
    ExtrusionRoles m_extrusion_roles;
    //
    // Detected extruders count
    //
    uint8_t m_extruders_count{ libpgcode::MIN_EXTRUDERS_COUNT };
    //
    // Detected options
    //
    libpgcode::OptionTypes m_options;
    //
    // Detected used extruders
    //
    Extruders m_used_extruders;
    //
    // Vertices ranges for visualization
    //
    ViewRange m_view_range;
    //
    // Detected total moves times
    //
    libpgcode::Times m_total_time{};
    //
    // Detected time modes
    //
    libpgcode::TimeModes m_time_modes;
    //
    // Detected options moves times
    //
    std::vector<std::pair<libpgcode::OptionType, libpgcode::Times>> m_options_times;
    //
    // List of gcode events
    //
    GCodeEvents m_gcode_events;
    //
    // Radius of cylinders used to render travel moves segments
    //
    float m_travels_radius{ DEFAULT_TRAVELS_RADIUS_MM };
    //
    // Radius of cylinders used to render wipe moves segments
    //
    float m_wipes_radius{ DEFAULT_WIPES_RADIUS_MM };
    //
    // Palette used to render extrusion roles
    //
    std::array<ColorRGB, libpgcode::GCODE_EXTRUSION_ROLES_COUNT> m_extrusion_roles_colors;
    //
    // Palette used to render options
    //
    std::array<ColorRGB, libpgcode::OPTION_TYPES_COUNT> m_options_colors;
    //
    // Lights used in rendering
    //
    Lights m_lights;

    bool m_initialized{ false };

    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;
    //
    // The OpenGL element used to represent all option markers
    //
    OptionTemplate m_option_template;
    //
    // The OpenGL element used to represent the center of gravity
    //
    CogMarker m_cog_marker;
    float m_cog_marker_scale_factor{ 1.0f };
    //
    // The OpenGL element used to represent the tool nozzle
    //
    ToolMarker m_tool_marker;
    float m_tool_marker_scale_factor{ 1.0f };
    //
    // cpu buffer to store vertices
    //
    libpgcode::MoveVertices m_vertices;

    // Cache for the colors to reduce the need to recalculate colors of all the vertices.
    std::vector<float> m_vertices_colors;

    //
    // Variables used for toolpaths visibiliity
    //
    BitSet<> m_valid_lines_bitset;
    //
    // Variables used for toolpaths coloring
    //
    std::optional<Settings> m_ranges_settings;
    ColorRange m_height_range;
    ColorRange m_width_range;
    ColorRange m_speed_range;
    ColorRange m_actual_speed_range;
    ColorRange m_fan_speed_range;
    ColorRange m_temperature_range;
    ColorRange m_volumetric_rate_range;
    ColorRange m_actual_volumetric_rate_range;
    std::array<ColorRange, COLOR_RANGE_TYPES_COUNT> m_layer_time_range{
        ColorRange(ColorRangeType::Linear), ColorRange(ColorRangeType::Logarithmic)
    };
    Palette m_tool_colors;
    Palette m_color_print_colors;

    App::Render::Device* m_device{ nullptr };
 
#if !USE_TEXTURE_BUFFER
    class TextureData
    {
    public:
        void init(App::Render::Device* device, size_t vertices_count);
        void set_positions(const std::vector<Vec4f>& positions);
        void set_heights_widths_angles(const std::vector<Vec4f>& heights_widths_angles);
        void set_colors(const std::vector<float>& colors);
        void set_enabled_segments(const std::vector<uint32_t>& enabled_segments);
        void set_enabled_options(const std::vector<uint32_t>& enabled_options);
        void reset();
        size_t count() const { return m_count; }
        std::pair<App::Render::Texture*, size_t> positions_tex(size_t id) const;
        std::pair<App::Render::Texture*, size_t> heights_widths_angles_tex(size_t id) const;
        std::pair<App::Render::Texture*, size_t> colors_tex(size_t id) const;
        std::pair<App::Render::Texture*, size_t> enabled_segments_tex(size_t id) const;
        std::pair<App::Render::Texture*, size_t> enabled_options_tex(size_t id) const;

        size_t max_texture_capacity() const { return m_width * m_height; }

    private:
        App::Render::Device* m_device{ nullptr };

        //
        // Texture width
        //
        size_t m_width{ 0 };
        //
        // Texture height
        //
        size_t m_height{ 0 };
        //
        // Count of textures
        //
        size_t m_count{ 0 };

        size_t m_max_texture_size{ 0 };

        struct Textures
        {
            std::pair<App::Render::Texture*, size_t> positions{ nullptr, 0 };
            std::pair<App::Render::Texture*, size_t> heights_widths_angles{ nullptr, 0 };
            std::pair<App::Render::Texture*, size_t> colors{ nullptr, 0 };
            std::pair<App::Render::Texture*, size_t> enabled_segments{ nullptr, 0 };
            std::pair<App::Render::Texture*, size_t> enabled_options{ nullptr, 0 };
        };

        std::vector<Textures> m_tex_ids;
    };

    TextureData m_texture_data;
#else
    std::unique_ptr<App::Render::TextureBuffer> m_positions_buffer;
    std::unique_ptr<App::Render::TextureBuffer> m_heights_widths_angles_buffer;
    std::unique_ptr<App::Render::TextureBuffer> m_colors_buffer;
    std::unique_ptr<App::Render::TextureBuffer> m_enabled_segments_buffer;
    std::unique_ptr<App::Render::TextureBuffer> m_enabled_options_buffer;
#endif // !USE_TEXTURE_BUFFER

    size_t m_enabled_segments_count{ 0 };
    size_t m_enabled_options_count{ 0 };

    void update_view_full_range();
    void update_color_ranges();
    void update_heights_widths();
    void render_segments(const Transform3f& view_matrix, const Transform3f& projection_matrix, const Vec3f& camera_position);
    void render_options(const Transform3f& view_matrix, const Transform3f& projection_matrix);
    void render_cog_marker(const Transform3f& view_matrix, const Transform3f& projection_matrix);
    void render_tool_marker(const Transform3f& view_matrix, const Transform3f& projection_matrix);
};

} // namespace Slic3r::Biz::libvgcode
