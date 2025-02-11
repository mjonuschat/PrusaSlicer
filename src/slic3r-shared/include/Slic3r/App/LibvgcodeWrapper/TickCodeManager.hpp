#pragma once

#include "Slic3r/App/LibvgcodeWrapper/ExtrudersSequence.hpp"

#include <libslic3r/CustomGCode.hpp>

#include <set>
#include <functional>
#include <array>

namespace Slic3r::App::LibvgcodeWrapper {

typedef std::function<void(Slic3r::CustomGCode::Type)> CheckGCodeCallback;
typedef std::function<std::string(const std::string&, float)> GetCustomGCodeCallback;
typedef std::function<std::string(const std::string&, float)> GetPausePrintMsgCallback;
typedef std::function<std::string(const std::string&)> GetNewColorCallback;
typedef std::function<int(const std::string&, int)> ShowInfoMsgCallback;
typedef std::function<std::string(Slic3r::CustomGCode::Type)> GetGCodeCallback;
typedef std::function<std::set<int>(float)> GetUsedExtrudersInPrintCallback;
typedef std::function<bool(ExtrudersSequence&)> GetExtrudersSequenceCallback;

struct TickCode
{
    bool operator<(const TickCode& other) const { return other.tick > this->tick; }
    bool operator>(const TickCode& other) const { return other.tick < this->tick; }

    int tick{ 0 };
    Slic3r::CustomGCode::Type type{ Slic3r::CustomGCode::Type::ColorChange };
    int extruder = 0;
    std::string color;
    std::string extra;
};

enum class ConflictType
{
    None,
    ModeConflict,
    MeaninglessColorChange,
    MeaninglessToolChange,
    NotPossibleToolChange,
    Redundant
};

class TickCodeManager
{
public:
    std::set<TickCode> ticks;
    Slic3r::CustomGCode::Mode mode{ Slic3r::CustomGCode::Mode::Undef };
    bool is_wipe_tower{ false }; //This flag indicates that there is multiple extruder print with wipe tower
    int only_extruder_id{ -1 };

    // colors per extruder
    std::vector<std::string> colors;

    TickCodeManager();
    ~TickCodeManager() = default;

    bool empty() const { return ticks.empty(); }

    void set_ticks(const Slic3r::CustomGCode::Info& custom_gcode_per_print_z);

    bool has_tick(int tick) const;
    bool add_tick(const int tick, Slic3r::CustomGCode::Type type, int extruder, float print_z);
    bool edit_tick(std::set<TickCode>::iterator it, float print_z);
    void add_auto_color_change(Slic3r::CustomGCode::Mode main_mode, const int extruders_cnt, float print_z);
    void switch_code(Slic3r::CustomGCode::Type type_from, Slic3r::CustomGCode::Type type_to);
    bool switch_code_for_tick(std::set<TickCode>::iterator it, Slic3r::CustomGCode::Type type_to, const int extruder);
    void erase_all_ticks_with_code(Slic3r::CustomGCode::Type type);

    ConflictType is_conflict_tick(const TickCode& tick, Slic3r::CustomGCode::Mode main_mode, float print_z) const;

    int tick_from_value(float value, bool force_lower_bound = false) const;

    std::string gcode(Slic3r::CustomGCode::Type type) const;

    // Get used extruders for tick.
    // Means all extruders(tools) which will be used during printing from current tick to the end
    std::set<int> used_extruders_for_tick(int tick, float print_z,
        Slic3r::CustomGCode::Mode force_mode = Slic3r::CustomGCode::Mode::Undef) const;

    // Get active extruders for tick. 
    // Means one current extruder for not existing tick OR 
    // 2 extruders - for existing tick (extruder before ToolChangeCode and extruder of current existing tick)
    // Use those values to disable selection of active extruders
    std::array<int, 2> active_extruders_for_tick(int tick, Slic3r::CustomGCode::Mode main_mode) const;

    std::string color_for_tool_change_tick(std::set<TickCode>::const_iterator it) const;
    std::string color_for_color_change_tick(std::set<TickCode>::const_iterator it) const;

    // true  -> if manipulation with ticks with selected type and in respect to the main_mode (slider mode) is possible
    // false -> otherwise
    bool check_ticks_changed_event(Slic3r::CustomGCode::Type type, Slic3r::CustomGCode::Mode main_mode);

    // return true, if extruder sequence was changed
    bool edit_extruder_sequence(const int max_tick, Slic3r::CustomGCode::Mode main_mode);

    void set_default_colors(bool default_colors_on) { m_use_default_colors = default_colors_on; }
    bool used_default_colors() const { return m_use_default_colors; }

    void set_values(const std::vector<float>* values) { m_values = values; }

    void set_check_gcode_callback(CheckGCodeCallback cb) { m_cb_check_gcode_and_notify = cb; }
    void set_get_custom_code_callback(GetCustomGCodeCallback cb) { m_cb_get_custom_code = cb; }
    void set_get_pause_print_msg_callback(GetPausePrintMsgCallback cb) { m_cb_get_pause_print_msg = cb; }
    void set_get_new_color_callback(GetNewColorCallback cb) { m_cb_get_new_color = cb; }
    void set_show_info_msg_callback(ShowInfoMsgCallback cb) { m_cb_show_info_msg = cb; }
    void set_get_gcode_callback(GetGCodeCallback cb) { m_cb_get_gcode = cb; }
    void set_get_used_extruders_in_print_callback(GetUsedExtrudersInPrintCallback cb) { m_cb_get_used_extruders_in_print = cb; }
    void set_get_extruders_sequence_callback(GetExtrudersSequenceCallback cb) { m_cb_get_extruders_sequence = cb; }

private:
    bool has_tick_with_code(Slic3r::CustomGCode::Type type);

    std::string color_for_tick(TickCode tick, Slic3r::CustomGCode::Type type, const int extruder);
    std::string custom_code(const std::string& code_in, float height);
    std::string pause_print_msg(const std::string& msg_in, float height);
    std::string new_color(const std::string& color);

private:
    std::string m_custom_gcode;
    std::string m_pause_print_msg;
    bool m_use_default_colors{ true };
    // pointer to the m_values from DSForLayers
    const std::vector<float>* m_values{ nullptr };
    ExtrudersSequence m_extruders_sequence;

    CheckGCodeCallback m_cb_check_gcode_and_notify{ nullptr };
    GetCustomGCodeCallback m_cb_get_custom_code{ nullptr };
    GetPausePrintMsgCallback m_cb_get_pause_print_msg{ nullptr };
    GetNewColorCallback m_cb_get_new_color{ nullptr };
    ShowInfoMsgCallback m_cb_show_info_msg{ nullptr };
    GetGCodeCallback m_cb_get_gcode{ nullptr };
    GetUsedExtrudersInPrintCallback m_cb_get_used_extruders_in_print{ nullptr };
    GetExtrudersSequenceCallback m_cb_get_extruders_sequence{ nullptr };
};

} // namespace Slic3r::App::LibvgcodeWrapper
