#pragma once

#include "Slic3r/App/Imgui/DoubleSlider.hpp"
#include "Slic3r/App/Imgui/RulerForDoubleSlider.hpp"
#include "Slic3r/App/Preview/TickCodeManager.hpp"
#include "Slic3r/Domain/Units.hpp"

namespace Slic3r::App::Preview {

typedef std::function<void(void)> TicksChangedCallback;
typedef std::function<void(const std::string&, const std::string&)> AppConfigChangedCallback;
typedef std::function<std::vector<std::string>(void)> GetExtruderColorsCallback;
typedef std::function<bool(void)> AutoColorChangeCallback;

enum class FocusedItem
{
    None,
    RevertIcon,
    OneLayerIcon,
    CogIcon,
    ColorBand,
    ActionIcon,
    SmartWipeTower,
    Tick
};

enum class DrawMode
{
    Regular,
    SlaPrint,
    SequentialFffPrint
};

enum class LabelType
{
    HeightWithLayer,
    Height,
    EstimatedTime,
};

class DoubleSliderForLayers : public Imgui::DoubleSlider::Manager<float>
{
public:
    DoubleSliderForLayers() = default;
    ~DoubleSliderForLayers() = default;

    void init(int lowerValue,
              int higherValue,
              int minValue,
              int maxValue,
              bool allow_editing);

    void change_one_layer_lock();

    Slic3r::Domain::CustomGCodeInfo ticks_values() const;
    void set_ticks_values(const Slic3r::Domain::CustomGCodeInfo& custom_gcode_per_print_z);

    void set_layers_times(const std::vector<float>& layers_times, float total_time);
    void set_layers_times(const std::vector<float>& layers_times);

    void set_draw_mode(bool is_sla_print, bool is_sequential_print);

    void set_mode_and_only_extruder(const bool is_one_extruder_printed_model, const int only_extruder);

    void force_ruler_update() { m_ruler.invalidate(); }

    // jump to selected layer
    void jump_to_value();

    // just for editor

    void set_extruder_colors(const std::vector<std::string>& extruder_colors) { m_ticks.colors = extruder_colors; }
    void use_default_colors(bool def_colors_on) { m_ticks.set_default_colors(def_colors_on); }
    bool is_new_print(const std::string& print_obj_idxs);
    void show_estimated_times(bool show) { m_show_estimated_times = show; }
    void show_ruler(bool show, bool show_bg) { m_show_ruler = show; m_show_ruler_bg = show_bg; }

    // manipulation with slider from keyboard

    // add default action for tick, when press "+"
    void add_current_tick();
    // delete current tick, when press "-"
    void delete_current_tick();
    // process adding of auto color change
    void auto_color_change();

    bool has_ticks() const { return !m_ticks.empty(); }
    void clear_ticks() { m_ticks.ticks.clear(); }

    void add_auto_color_change(const int extruders_cnt, const float print_z) { m_ticks.add_auto_color_change(m_mode, extruders_cnt, print_z); }
    bool is_auto_color_change_completed() const {
        // allow max 3 auto color changes
        return m_ticks.ticks.size() > 2;
    }

    void set_ticks_changed_callback(TicksChangedCallback cb) { m_cb_ticks_changed = cb; };
    void set_app_config_changed_callback(AppConfigChangedCallback cb) { m_cb_app_config_changed = cb; }
    void set_get_extruder_colors_callback(GetExtruderColorsCallback cb) { m_cb_get_extruder_colors = cb; }
    void set_auto_color_change_callback(AutoColorChangeCallback cb) { m_cb_auto_color_change = cb; }
    void set_check_gcode_callback(CheckGCodeCallback cb) { m_ticks.set_check_gcode_callback(cb); }
    void set_get_custom_code_callback(GetCustomGCodeCallback cb) { m_ticks.set_get_custom_code_callback(cb); }
    void set_get_pause_print_msg_callback(GetPausePrintMsgCallback cb) { m_ticks.set_get_pause_print_msg_callback(cb); }
    void set_get_new_color_callback(GetNewColorCallback cb) { m_ticks.set_get_new_color_callback(cb); }
    void set_show_info_msg_callback(ShowInfoMsgCallback cb) { m_ticks.set_show_info_msg_callback(cb); }
    void set_get_gcode_callback(GetGCodeCallback cb) { m_ticks.set_get_gcode_callback(cb); }
    void set_get_used_extruders_in_print_callback(GetUsedExtrudersInPrintCallback cb) { m_ticks.set_get_used_extruders_in_print_callback(cb); }
    void set_get_extruders_sequence_callback(GetExtrudersSequenceCallback cb) { m_ticks.set_get_extruders_sequence_callback(cb); }

    std::string gcode(Slic3r::Domain::CustomGCodeType type) const { return m_ticks.gcode(type); }

    const ImVec2 get_size() const { return m_size; }

    void set_units(Slic3r::Domain::UnitsSystem units) { m_units = units; }

    /**
     * @name Implementation of Imgui::DoubleSlider::Manager public interface
     * @{
     */
    void render(float scale_factor = 1.0f, float offset = 0.0f) override;
    /**@}*/

private:
    bool is_wipe_tower_layer(int tick) const;

    std::string label(int tick, LabelType label_type, unsigned int decimals = 2) const;

    std::string tooltip(int tick = -1) const;

    void update_draw_scroll_line_cb();

    // functions for extend rendering of m_ctrl

    void draw_colored_band(const ImRect& groove, const ImRect& slideable_region);
    void draw_ticks(const ImRect& slideable_region);
    void draw_ruler(const ImRect& slideable_region);
    void render_menu();
    void render_cog_menu();
    void render_edit_menu();
    bool render_button(wchar_t icon, wchar_t icon_hovered, const std::string& label_id, const ImVec2& pos, FocusedItem focus, int tick = -1);
    void render_add_tick_menu();
    bool render_multi_extruders_menu(bool switch_current_code = false);
    bool render_jump_to_window(const ImVec2& pos, float& active_value);
    void render_color_picker();

    void add_code_as_tick(Slic3r::Domain::CustomGCodeType type, int selected_extruder = -1);
    void edit_tick(int tick = -1);
    void discard_all_ticks();
    void process_jump_to_value();
    bool can_edit() const { return m_allow_editing && m_draw_mode != DrawMode::SlaPrint; }

    /**
     * @name Implementation of Imgui::DoubleSlider::Manager private interface
     * @{
     */
    std::string label(int pos) const override { return label(pos, LabelType::HeightWithLayer); }
    /**@}*/

    void process_ticks_changed() { if (m_cb_ticks_changed != nullptr) m_cb_ticks_changed(); }

private:
    Slic3r::Domain::UnitsSystem m_units{ Slic3r::Domain::UnitsSystem::SI };

    bool m_is_osx{ false };
    bool m_allow_editing{ true };
    bool m_show_estimated_times{ true };
    bool m_show_ruler{ false };
    bool m_show_ruler_bg{ true };
    bool m_show_cog_menu{ false };
    bool m_show_edit_menu{ false };
    int m_pos_on_move{ -1 };

    DrawMode m_draw_mode{ DrawMode::Regular };
    Slic3r::Domain::PrinterMode m_mode{ Slic3r::Domain::PrinterMode::SingleExtruder };
    FocusedItem m_focus{ FocusedItem::None };

    Imgui::DoubleSlider::Ruler m_ruler;
    TickCodeManager m_ticks;
    float m_icon_screen_size{ 20.0f };
    ImVec2 m_size{ 0.0f, 0.0f };

    std::vector<float> m_layers_times;
    std::vector<float> m_layers_values;

    float m_jump_to_value{ 0.0f };

    bool m_show_just_color_change_menu{ false };
    bool m_show_get_jump_value{ false };
    bool m_show_color_picker{ false };

    std::string m_print_obj_idxs;
    std::string m_selectable_color;

    TicksChangedCallback m_cb_ticks_changed{ nullptr };
    GetExtruderColorsCallback m_cb_get_extruder_colors{ nullptr };
    AutoColorChangeCallback m_cb_auto_color_change{ nullptr };
    AppConfigChangedCallback m_cb_app_config_changed{ nullptr };
};

} // namespace Slic3r::App::Preview
