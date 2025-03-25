#include "Slic3r/App/LibvgcodeWrapper/Legend.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperImpl.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include <Slic3r/Biz/libpgcode/Utils.hpp>
#include <Slic3r/App/libvgcode/Viewer.hpp>
#include <Slic3r/App/libvgcode/ColorRange.hpp>

#include <boost/nowide/convert.hpp>

using namespace Slic3r::App::libvgcode;
using namespace Slic3r::Biz::libpgcode;
using namespace Slic3r::Biz;
using namespace Slic3r::Domain;

namespace Slic3r::App::LibvgcodeWrapper {

void legend_view_type_selector(Viewer& viewer, const WrapperImpl& wrapper, GCodeViewTypeChangedCallback cb_view_type_changed,
    float width)
{
    std::vector<float> layers_times = viewer.layers_estimated_times();
    bool has_layers_times = !layers_times.empty() && layers_times.size() == viewer.layers_count();
    std::vector<int> layer_times_ids = { int(ViewType::LayerTimeLinear), int(ViewType::LayerTimeLogarithmic) };

    std::vector<int> options_id;
    options_id.reserve(VIEW_TYPES_COUNT);
    for (int i = 0; i < int(VIEW_TYPES_COUNT); ++i) {
        if (has_layers_times ||
            std::find(layer_times_ids.begin(), layer_times_ids.end(), i) == layer_times_ids.end())
            options_id.emplace_back(i);
    }

    int selection = int(viewer.view_type());
    int old_selection = selection;
    if (!has_layers_times &&
        std::find(layer_times_ids.begin(), layer_times_ids.end(), selection) != layer_times_ids.end())
        selection = int(ViewType::FeatureType);
    int selection_id = int(std::distance(options_id.begin(), std::find(options_id.begin(), options_id.end(), selection)));

    ImGui::PushItemWidth(width);
    if (ImGui::BeginCombo("##ViewTypeSelector", to_string(ViewType(options_id[selection_id])).c_str(),
        ImGuiComboFlags_HeightLargest)) {
        for (int i = 0; i < int(options_id.size()); ++i) {
            if (ImGui::Selectable(to_string(ViewType(options_id[i])).c_str(), i == selection))
                selection = options_id[i];
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (old_selection != selection) {
        viewer.set_view_type(ViewType(selection));
        if (cb_view_type_changed != nullptr)
            cb_view_type_changed();
    }
}

static void draw_feature_type_items(Viewer& viewer, WrapperImpl& wrapper, const LegendCallbacks& cbs)
{
    static constexpr float max_percentage_rect_width = 30.0f;
    static bool show_time_estimate = true;

    GCodeExtrusionRoles roles = viewer.extrusion_roles();
    float total_time = viewer.estimated_time();
    float inv_total_time = 100.0f / total_time;
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);

    if (ImGui::BeginTable("FeatureTypeItems", 4)) {

        // prevent header highlight while hovering or clicking on it
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
        
        ImGui::TableSetupColumn("");
        ImGui::TableSetupColumn(_u8L("Feature").c_str());
        ImGui::TableSetupColumn(show_time_estimate ? _u8L("Time").c_str() : _u8L("Used filament").c_str());
        ImGui::TableSetupColumn(_u8L("Percentage").c_str());
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGui::PopStyleColor(2);

        float max_time_percentage = 0.0f;
        std::vector<std::pair<float, float>> times_percentages;
        float travels_time = 0.0f;
        float travels_time_percentage = 0.0f;
        float wipes_time = 0.0f;
        float wipes_time_percentage = 0.0f;

        float max_length_percentage = 0.0f;
        std::vector<std::pair<float, float>> length_percentages;

        if (show_time_estimate) {
            times_percentages.reserve(roles.size());
            for (size_t i = 0; i < roles.size(); ++i) {
                const GCodeExtrusionRole& role = roles[i];
                float time = viewer.extrusion_role_estimated_time(role);
                float percentage = time * inv_total_time;
                times_percentages.emplace_back(time, percentage);
                max_time_percentage = std::max(max_time_percentage, percentage);
            }
            travels_time = viewer.option_estimated_time(OptionType::Travels);
            travels_time_percentage = travels_time * inv_total_time;
            max_time_percentage = std::max(max_time_percentage, travels_time_percentage);
            wipes_time = viewer.option_estimated_time(OptionType::Wipes);
            wipes_time_percentage = wipes_time * inv_total_time;
            max_time_percentage = std::max(max_time_percentage, wipes_time_percentage);
        }
        else {
            float total_length = 0.0f;
            for (GCodeExtrusionRole role : roles) {
                total_length += viewer.extrusion_role_used_filament_length(role);
            }
            float inv_total_length = 100.0f / total_length;

            length_percentages.reserve(roles.size());
            for (GCodeExtrusionRole role : roles) {
                float length = viewer.extrusion_role_used_filament_length(role);
                float percentage = length * inv_total_length;
                length_percentages.emplace_back(length, percentage);
                max_length_percentage = std::max(max_length_percentage, percentage);
            }
        }

        for (size_t i = 0; i < roles.size(); ++i) {
            GCodeExtrusionRole role = roles[i];
            ColorRGB color = viewer.extrusion_role_color(role);
            std::string role_name = to_string(role);
            bool visible = viewer.is_extrusion_role_visible(role);
            uint8_t alpha = visible ? 255 : 0;

            if (!visible)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImVec2 row_pos = ImGui::GetCursorScreenPos();
            ImGui::RenderFrame(row_pos + ImVec2(1.0f, 1.0f), row_pos + icon_size, Imgui::to_ImU32(color, alpha));
            ImGui::Dummy(icon_size);

            // highlight row when hovering and process click on row
            ImGui::SetCursorScreenPos(row_pos);
            if (ImGui::Selectable(("##" + role_name).c_str(), false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                viewer.toggle_extrusion_role_visibility(role);
                if (cbs.cb_extrusion_role_visibility_changed != nullptr)
                    cbs.cb_extrusion_role_visibility_changed();
            }

            if (ImGui::IsItemHovered()) {
                if (!visible) ImGui::PopStyleColor();
                Imgui::tooltip(visible ? _u8L("Click to hide") : _u8L("Click to show"));
                if (!visible) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", role_name.c_str());

            ImGui::TableSetColumnIndex(2);
            if (show_time_estimate)
                ImGui::Text("%s", format_time_dhms_short(times_percentages[i].first).c_str());
            else {
                std::string txt_length = convert_and_format_units(length_percentages[i].first,
                    UnitsType::Meters, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Meters : UnitsType::Feet, 2);
                std::string txt_mass = convert_and_format_units(viewer.extrusion_role_used_filament_mass(role),
                    UnitsType::Grams, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Grams : UnitsType::Ounces, 2);
                ImGui::Text("%s (%s)", txt_length.c_str(), txt_mass.c_str());
            }

            ImGui::TableSetColumnIndex(3);
            ImVec2 rect_pos = ImGui::GetCursorScreenPos();
            if (show_time_estimate) {
                float rect_width = max_percentage_rect_width * times_percentages[i].second / max_time_percentage;
                ImGui::RenderFrame(rect_pos + ImVec2(1.0f, 1.0f), rect_pos + ImVec2(rect_width, line_height),
                    ImGui::GetColorU32(ImGuiCol_ButtonActive, 1.0f));
                ImGui::Dummy({ max_percentage_rect_width, line_height });
                ImGui::SameLine();
                ImGui::Text("%.1f%%", times_percentages[i].second);
            }
            else {
                float rect_width = max_percentage_rect_width * length_percentages[i].second / max_length_percentage;
                ImGui::RenderFrame(rect_pos + ImVec2(1.0f, 1.0f), rect_pos + ImVec2(rect_width, line_height),
                    ImGui::GetColorU32(ImGuiCol_ButtonActive, 1.0f));
                ImGui::Dummy({ max_percentage_rect_width, line_height });
                ImGui::SameLine();
                ImGui::Text("%.1f%%", length_percentages[i].second);
            }

            if (!visible)
                ImGui::PopStyleColor();
        }

        if (show_time_estimate) {
            std::vector<std::pair<OptionType, std::pair<float, float>>> options = {
                { OptionType::Travels, { travels_time, travels_time_percentage } },
                { OptionType::Wipes,   { wipes_time, wipes_time_percentage } }
            };

            for (const auto& [option, times] : options) {
                const auto& [time, percentage] = times;
                if (viewer.is_option_visible(option)) {
                    ColorRGB color = viewer.option_color(option);
                    std::string name = to_string(option);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImVec2 row_pos = ImGui::GetCursorScreenPos();
                    ImGui::RenderFrame(row_pos + ImVec2(1.0f, 1.0f), row_pos + icon_size, Imgui::to_ImU32(color));
                    ImGui::Dummy(icon_size);

                    // highlight row when hovering and process click on row
                    ImGui::SetCursorScreenPos(row_pos);
                    if (ImGui::Selectable(("##" + name).c_str(), false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                        MoveType type = MoveType::COUNT;
                        if (option == OptionType::Travels)
                            type = MoveType::Travel;
                        else if (option == OptionType::Wipes)
                            type = MoveType::Wipe;
                        if (type != MoveType::COUNT)
                            wrapper.set_radius_popup_type(type);
                    }

                    if (ImGui::IsItemHovered())
                       Imgui::tooltip(_u8L("Click to edit thickness"));

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", name.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", format_time_dhms(time).c_str());

                    ImGui::TableSetColumnIndex(3);
                    ImVec2 rect_pos = ImGui::GetCursorScreenPos();
                    if (show_time_estimate) {
                        float rect_width = max_percentage_rect_width * percentage / max_time_percentage;
                        ImGui::RenderFrame(rect_pos + ImVec2(1.0f, 1.0f), rect_pos + ImVec2(rect_width, line_height),
                            ImGui::GetColorU32(ImGuiCol_ButtonActive, 1.0f));
                        ImGui::Dummy({ max_percentage_rect_width, line_height });
                        ImGui::SameLine();
                        ImGui::Text("%.1f%%", percentage);
                    }
                }
            }
        }

        ImGui::EndTable();
    }

    if (ImGui::Button(show_time_estimate ? _u8L("Show used filament").c_str() : _u8L("Show time estimate").c_str(),
        { -1.0f, 0.0f })) {
        show_time_estimate = !show_time_estimate;
        if (cbs.cb_request_extra_frame != nullptr)
            cbs.cb_request_extra_frame(1);
    }
}

static void draw_color_range_items(const Viewer& viewer, const WrapperImpl& wrapper)
{
    ViewType type = viewer.view_type();
    const ColorRange& range = viewer.color_range(type);
    std::vector<float> values = range.values();
    if (values.empty())
        return;

    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);

    if (ImGui::BeginTable("ColorRangeItems", 2)) {

        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            const ColorRGB& color = range.color_at(*it);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(color));
            ImGui::Dummy(icon_size);

            ImGui::TableSetColumnIndex(1);
            switch (type)
            {
            case ViewType::Height:
            case ViewType::Width:
            {
                ImGui::Text("%s", convert_and_format_units(*it, UnitsType::Millimeters,
                    (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches, 3, false).c_str());
                break;
            }
            case ViewType::Speed:
            case ViewType::ActualSpeed:
            {
                ImGui::Text("%s", convert_and_format_units(*it, UnitsType::MillimetersPerSecond,
                    (wrapper.units() == UnitsSystem::SI) ? UnitsType::MillimetersPerSecond : UnitsType::InchesPerSecond, 1, false).c_str());
                break;
            }
            case ViewType::VolumetricFlowRate:
            case ViewType::ActualVolumetricFlowRate:
            {
                unsigned int decimals = (wrapper.units() == UnitsSystem::SI) ? 3 : 6;
                ImGui::Text("%s", convert_and_format_units(*it, UnitsType::MillimetersCubePerSecond,
                    (wrapper.units() == UnitsSystem::SI) ? UnitsType::MillimetersCubePerSecond : UnitsType::InchesCubePerSecond, decimals, false).c_str());
                break;
            }
            case ViewType::FanSpeed:
            {
                ImGui::Text("%.0f", *it);
                break;
            }
            case ViewType::Temperature:
            {
                ImGui::Text("%s", convert_and_format_units(*it, UnitsType::Celsius,
                    (wrapper.units() == UnitsSystem::SI) ? UnitsType::Celsius : UnitsType::Farhenheit, 0, false).c_str());
                break;
            }
            case ViewType::LayerTimeLinear:
            case ViewType::LayerTimeLogarithmic:
            {
                ImGui::Text("%s", format_time_dhms(*it).c_str());
                break;
            }
            default:
            {
                ImGui::Text("Error");
                break;
            }
            }
        }

        ImGui::EndTable();
    }
}

static void draw_tool_items(const Viewer& viewer, const WrapperImpl& wrapper)
{
    const std::vector<uint8_t>& used_extruders_ids = viewer.used_extruders_ids();
    const Palette& tool_colors = viewer.tool_colors();
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);

    if (ImGui::BeginTable("ToolItems", 3)) {

        // prevent header highlight while hovering or clicking on it
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));

        ImGui::TableSetupColumn("");
        ImGui::TableSetupColumn(_u8L("Extruder").c_str());
        ImGui::TableSetupColumn(_u8L("Used Filament").c_str());
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGui::PopStyleColor(2);

        for (size_t i = 0; i < used_extruders_ids.size(); ++i) {
            uint8_t extruder_id = used_extruders_ids[i];
            const ColorRGB& color = tool_colors[extruder_id];

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(color));
            ImGui::Dummy({ line_height, line_height });

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s %d", _u8L("Extruder").c_str(), 1 + extruder_id);

            ImGui::TableSetColumnIndex(2);
            std::string txt_length = convert_and_format_units(viewer.used_extruder_used_filament_length(extruder_id),
                UnitsType::Meters, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Meters : UnitsType::Feet,
                2);
            std::string txt_mass = convert_and_format_units(viewer.used_extruder_used_filament_mass(extruder_id),
                UnitsType::Grams, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Grams : UnitsType::Ounces,
                2);
            ImGui::Text("%s (%s)", txt_length.c_str(), txt_mass.c_str());
        }

        ImGui::EndTable();
    }
}

static bool has_gcode_events_to_show(const Viewer& viewer)
{
    if (viewer.used_extruders_count() > 1)
        return false;

    const std::vector<GCodeEvent>& events = viewer.gcode_events();
    if (events.empty())
        return false;
    for (const auto& item : events) {
        switch (item.type)
        {
        case CustomGCode::Type::PausePrint:
        case CustomGCode::Type::ColorChange: { return true; }
        default: { break; }
        }
    }
    return false;
}

static void draw_color_print_items(const Viewer& viewer, const WrapperImpl& wrapper,
    Imgui::DoubleSlider::RequestExtraFramesCallback cb = nullptr)
{
    static bool show_time_estimate = true;

    const Palette& color_print_colors = viewer.color_print_colors();
    std::vector<uint8_t> used_extruders_ids = viewer.used_extruders_ids();
    uint8_t extruders_count = viewer.extruders_count();
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);

    if (ImGui::BeginTable("ColorPrintItems", 2)) {

        for (uint8_t id : used_extruders_ids) {
            const std::vector<ColorPrint>& extr_color_prints = viewer.extruder_color_prints(id);
            if (extr_color_prints.size() == 1) {
                const ColorRGB& color = color_print_colors[id];

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(color));
                ImGui::Dummy(icon_size);

                ImGui::TableSetColumnIndex(1);
                if (extruders_count > 1)
                    ImGui::Text("%s %d %s", _u8L("Extruder").c_str(), 1 + id, _u8L("default color").c_str());
                else
                    ImGui::Text("%s", _u8L("Default color").c_str());
            }
            else {
                size_t counter = 0;
                for (auto it = extr_color_prints.rbegin(); it != extr_color_prints.rend(); ++it) {
                    const ColorRGB& color = color_print_colors[it->color_id % color_print_colors.size()];

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(color));
                    ImGui::Dummy(icon_size);

                    ImGui::TableSetColumnIndex(1);
                    if (counter == 0) {
                        if (extruders_count == 1) {
                            std::string txt = convert_and_format_units(viewer.layer_z(it->layer_id),
                                UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches,
                                2);
                            ImGui::Text("%s %s", _u8L("above").c_str(), txt.c_str());
                        }
                        else {
                            std::string txt = convert_and_format_units(viewer.layer_z(it->layer_id),
                                UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches,
                                2);
                            ImGui::Text("%s %d %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                                _u8L("above").c_str(), txt.c_str());
                        }
                    }
                    else if (counter == extr_color_prints.size() - 1) {
                        if (extruders_count == 1) {
                            std::string txt = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                                UnitsType::Millimeters, 
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches,
                                2);
                            ImGui::Text("%s %s", _u8L("up to").c_str(), txt.c_str());
                        }
                        else {
                            std::string txt = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                                UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches,
                                2);
                            ImGui::Text("%s %d %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                                _u8L("up to").c_str(), txt.c_str());
                        }
                    }
                    else {
                        if (extruders_count == 1) {
                            UnitsType length_units = (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches;
                            std::string txt_from = convert_and_format_units(viewer.layer_z(it->layer_id),
                                UnitsType::Millimeters, length_units, 2, false);
                            std::string txt_to = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                                UnitsType::Millimeters, length_units, 2);
                            ImGui::Text("%s %s %s %s",
                                _u8L("from").c_str(), txt_from.c_str(),
                                _u8L("to").c_str(), txt_to.c_str());
                        }
                        else {
                            UnitsType length_units = (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches;
                            std::string txt_from = convert_and_format_units(viewer.layer_z(it->layer_id),
                                UnitsType::Millimeters, length_units, 2, false);
                            std::string txt_to = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                                UnitsType::Millimeters, length_units, 2);
                            ImGui::Text("%s %d %s %s %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                                _u8L("from").c_str(), txt_from.c_str(),
                                _u8L("to").c_str(), txt_to.c_str());
                        }
                    }
                    ++counter;
                }
            }
        }

        ImGui::EndTable();
    }

    if (has_gcode_events_to_show(viewer)) {
        ImGui::Separator();

        if (ImGui::BeginTable("CustomGCodeEvents", show_time_estimate ? 3 : 2)) {
            // prevent header highlight while hovering or clicking on it
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));

            ImGui::TableSetupColumn(_u8L("Event").c_str());
            if (show_time_estimate) {
                ImGui::TableSetupColumn(_u8L("Duration").c_str());
                ImGui::TableSetupColumn(_u8L("Remaining time").c_str());
            }
            else
                ImGui::TableSetupColumn(_u8L("Used filament").c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGui::PopStyleColor(2);

            std::map<uint8_t, size_t> last_color_id;
            for (uint8_t id : used_extruders_ids) {
                last_color_id[id] = 0;
            }

            enum class EEventType : uint8_t
            {
                Print,
                ColorChange,
                Pause,
                Other
            };

            struct EventItem
            {
                EEventType type;
                uint8_t extruder_id{ 0 };
                std::optional<ColorRGB> color_1;
                std::optional<ColorRGB> color_2;
                std::array<float, 2> times{ 0.0f, 0.0f };
                std::array<float, 2> used_filament{ 0.0f, 0.0f };

                std::string get_label() const {
                    switch (type)
                    {
                    case EEventType::Print:       { return _u8L("Print"); }
                    case EEventType::ColorChange: { return _u8L("Color change"); }
                    case EEventType::Pause:       { return _u8L("Pause"); }
                    default:                      { return _u8L("Error"); }
                    }
                }
            };

            size_t time_mode = size_t(viewer.time_mode());
            const std::vector<GCodeEvent>& events = viewer.gcode_events();
            std::vector<EventItem> event_items;
            uint8_t extruder_id = events.front().extruder_id;
            event_items.push_back({ EEventType::Print, extruder_id,
                color_print_colors[viewer.extruder_color_prints(extruder_id)[last_color_id[extruder_id]].color_id] });
            for (size_t i = 0; i < events.size(); ++i) {
                const auto& item = events[i];
                switch (item.type)
                {
                case CustomGCode::Type::PausePrint:
                {
                    auto it = std::find_if(event_items.rbegin(), event_items.rend(), [](const EventItem& e) { return e.type == EEventType::Print; });
                    if (it != event_items.rend())
                        it->times[0] += item.times[time_mode];
                    extruder_id = item.extruder_id;
                    event_items.push_back({ EEventType::Pause, extruder_id, std::nullopt, std::nullopt });
                    event_items.push_back({ EEventType::Print, extruder_id,
                        color_print_colors[viewer.extruder_color_prints(extruder_id)[last_color_id[extruder_id]].color_id] });
                    break;
                }
                case CustomGCode::Type::ColorChange:
                {
                    auto it = std::find_if(event_items.rbegin(), event_items.rend(), [](const EventItem& e) { return e.type == EEventType::Print; });
                    if (it != event_items.rend()) {
                        it->times[0] += item.times[time_mode];
                        it->used_filament = item.used_filament;
                    }
                    extruder_id = item.extruder_id;
                    event_items.push_back({ EEventType::ColorChange, extruder_id,
                        color_print_colors[viewer.extruder_color_prints(extruder_id)[last_color_id[extruder_id]].color_id],
                        color_print_colors[viewer.extruder_color_prints(extruder_id)[last_color_id[extruder_id] + 1].color_id] });
                    ++last_color_id[extruder_id];
                    event_items.push_back({ EEventType::Print, item.extruder_id,
                        color_print_colors[viewer.extruder_color_prints(extruder_id)[last_color_id[extruder_id]].color_id] });
                    break;
                }
                default: {
                    auto it = std::find_if(event_items.rbegin(), event_items.rend(), [](const EventItem& e) { return e.type == EEventType::Print; });
                    if (it != event_items.rend())
                        it->times[0] += item.times[time_mode];
                    event_items.push_back({ EEventType::Other, item.extruder_id });
                    break;
                }
                }
            }

            float total_time = viewer.estimated_time();
            float remaining_time = total_time;
            for (auto& item : event_items) {
                item.times[1] = remaining_time;
                remaining_time -= item.times[0];
            }

            size_t max_extruder_id = size_t(*std::max_element(used_extruders_ids.begin(), used_extruders_ids.end()));
            std::vector<std::array<float, 2>> used_filament(1 + max_extruder_id, { 0.0f, 0.0f });
            for (auto& item : event_items) {
                if (item.type == EEventType::Print) {
                    used_filament[item.extruder_id][0] += item.used_filament[0];
                    used_filament[item.extruder_id][1] += item.used_filament[1];
                }
            }

            EventItem& last_event = event_items.back();
            last_event.times[0] = remaining_time;
            last_event.used_filament[0] = viewer.used_extruder_used_filament_length(last_event.extruder_id) - used_filament[last_event.extruder_id][0];
            last_event.used_filament[1] = viewer.used_extruder_used_filament_mass(last_event.extruder_id) - used_filament[last_event.extruder_id][1];

            for (const auto& item : event_items) {
                if (item.type == EEventType::Other)
                    continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                std::string label = item.get_label();
                ImGui::Text("%s", label.c_str());
                if (item.color_1.has_value()) {
                    const ImGuiStyle& style = ImGui::GetStyle();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    pos.x += style.FramePadding.x + ImGui::CalcTextSize(label.c_str()).x + style.ItemInnerSpacing.x;
                    pos.y -= style.FramePadding.y + style.ItemInnerSpacing.y + line_height;
                    ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(*item.color_1));
                    ImGui::SameLine();
                    ImGui::Dummy(icon_size);
                    if (item.color_2.has_value()) {
                        pos.x += line_height;
                        ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(*item.color_2));
                        ImGui::SameLine();
                        ImGui::Dummy(icon_size);
                    }
                }

                if (show_time_estimate) {
                    if (item.times[0] > 0.0f) {
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", format_time_dhms_short(item.times[0]).c_str());
                    }
                    if (item.times[1] > 0.0f) {
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", format_time_dhms_short(item.times[1]).c_str());
                    }
                }
                else {
                    if (item.used_filament[0] > 0.0f) {
                        ImGui::TableSetColumnIndex(1);
                        std::string txt_length = convert_and_format_units(item.used_filament[0],
                            UnitsType::Meters, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Meters : UnitsType::Feet,
                            2);
                        std::string txt_mass = convert_and_format_units(item.used_filament[1],
                            UnitsType::Grams, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Grams : UnitsType::Ounces,
                            2);
                        ImGui::Text("%s (%s)", txt_length.c_str(), txt_mass.c_str());
                    }
                }
            }

            ImGui::EndTable();
        }

        if (ImGui::Button(show_time_estimate ? _u8L("Show used filament").c_str() : _u8L("Show time estimate").c_str(),
            { -1.0f, 0.0f })) {
            show_time_estimate = !show_time_estimate;
            if (cb != nullptr)
                cb(1);
        }
    }
}

static void draw_type_items(Viewer& viewer, WrapperImpl& wrapper, const LegendCallbacks& cbs)
{
    ViewType type = viewer.view_type();
    switch (type)
    {
    case ViewType::FeatureType:              { draw_feature_type_items(viewer, wrapper, cbs); break; }
    case ViewType::Height:
    case ViewType::Width:
    case ViewType::ActualSpeed:
    case ViewType::Speed:
    case ViewType::FanSpeed:
    case ViewType::Temperature:
    case ViewType::VolumetricFlowRate:
    case ViewType::ActualVolumetricFlowRate:
    case ViewType::LayerTimeLinear:
    case ViewType::LayerTimeLogarithmic:     { draw_color_range_items(viewer, wrapper); break; }
    case ViewType::Tool:                     { draw_tool_items(viewer, wrapper); break; }
    case ViewType::ColorPrint:               { draw_color_print_items(viewer, wrapper, cbs.cb_request_extra_frame); break; }
    default:                                 { break; }
    }

    switch (type)
    {
    case ViewType::Width:
    case ViewType::VolumetricFlowRate:
    case ViewType::ActualVolumetricFlowRate:
    {
        std::string label = viewer.is_extrusion_role_visible(GCodeExtrusionRole::Custom) ?
            _u8L("Hide Custom G-code") : _u8L("Show Custom G-code");
        if (ImGui::Button(label.c_str(), { -1.f, 0.0f }))
            viewer.toggle_extrusion_role_visibility(GCodeExtrusionRole::Custom);
        break;
    }
    default: { break; }
    }
}

static wchar_t icon_id(OptionType type)
{
    switch (type)
    {
    case OptionType::Travels:         { return ImGui::LegendTravel; }
    case OptionType::Wipes:           { return ImGui::LegendWipe; }
    case OptionType::Retractions:     { return ImGui::LegendRetract; }
    case OptionType::Unretractions:   { return ImGui::LegendDeretract; }
    case OptionType::Seams:           { return ImGui::LegendSeams; }
    case OptionType::ToolChanges:     { return ImGui::LegendToolChanges; }
    case OptionType::ColorChanges:    { return ImGui::LegendColorChanges; }
    case OptionType::PausePrints:     { return ImGui::LegendPausePrints; }
    case OptionType::CustomGCodes:    { return ImGui::LegendCustomGCodes; }
    case OptionType::CenterOfGravity: { return ImGui::LegendCOG; }
    case OptionType::ToolMarker:      { return ImGui::LegendToolMarker; }
    default:                          { return L'\0'; }
    }
}

static void draw_options(Viewer& viewer, WrapperImpl& wrapper, Imgui::DoubleSlider::RequestExtraFramesCallback cb = nullptr)
{
    if (viewer.options_count() == 0)
        return;

    float line_height = ImGui::GetTextLineHeight();

    OptionTypes options = viewer.options();
    options.push_back(OptionType::CenterOfGravity);
    options.push_back(OptionType::ToolMarker);

    CustomOptions dummy_options;
    CustomOptions& custom_options = (wrapper.mode() == WrapperMode::GCodeViewer) ? dummy_options : wrapper.custom_options();

    float icon_size = line_height;

    ImGui::PushItemWidth(-1.0f);
    if (ImGui::BeginCombo("##options", _u8L("Options").c_str(), ImGuiComboFlags_HeightLargest)) {
        for (const OptionType option : options) {

            if (option == OptionType::Retractions || option == OptionType::CenterOfGravity)
                ImGui::Separator();

            std::string option_name = to_string(option);
            bool visible = viewer.is_option_visible(option);
            ImVec2 pos = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable(("##" + option_name).c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
                viewer.toggle_option_visibility(option);
                if (cb != nullptr)
                    cb(1);
            }

            ImGui::SetCursorScreenPos(pos);

            Imgui::icon_image(icon_id(option), { icon_size, icon_size });

            pos.x += icon_size + ImGui::GetStyle().ItemInnerSpacing.x;
            ImRect check_bb(pos, pos + ImVec2(icon_size, icon_size));
            float pad = ImMax(1.0f, IM_FLOOR(icon_size / 6.0f));
            ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
            if (visible)
                ImGui::RenderCheckMark(ImGui::GetWindowDrawList(), check_bb.Min + ImVec2(pad, pad), ImGui::GetColorU32(ImGuiCol_CheckMark),
                    icon_size - pad * 2.0f);
            ImGui::SameLine();
            ImGui::Dummy({ icon_size, icon_size });

            ImGui::SameLine();
            ImGui::Text("%s", option_name.c_str());
        }

        if (!custom_options.empty())
            ImGui::Separator();
        for (size_t i = 0; i < custom_options.size(); ++i ) {
            CustomOption& option = custom_options[i];
            ImVec2 pos = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable(("##" + option.name).c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
                option.visible = !option.visible;
                if (option.cb_action != nullptr)
                    option.cb_action(option.visible);
                if (cb != nullptr)
                    cb(1);
            }

            ImGui::SetCursorScreenPos(pos);

            Imgui::icon_image(option.icon, { icon_size, icon_size });

            pos.x += icon_size + ImGui::GetStyle().ItemInnerSpacing.x;
            ImRect check_bb(pos, pos + ImVec2(icon_size, icon_size));
            float pad = ImMax(1.0f, IM_FLOOR(icon_size / 6.0f));
            ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
            if (option.visible)
                ImGui::RenderCheckMark(ImGui::GetWindowDrawList(), check_bb.Min + ImVec2(pad, pad), ImGui::GetColorU32(ImGuiCol_CheckMark),
                icon_size - pad * 2.0f);
            ImGui::SameLine();
            ImGui::Dummy({ icon_size, icon_size });

            ImGui::SameLine();
            ImGui::Text("%s", option.name.c_str());
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

static void draw_producer(GCodeProducer producer)
{
    if (producer == GCodeProducer::PrusaSlicer)
        return;

    ImGui::SeparatorText(_u8L("Producer").c_str());
    ImGui::Text("%s", std::string(producer_name(producer)).c_str());
}

static std::string trim_text_if_needed(const std::string& txt, float max_length = 200.0f)
{
    float length = ImGui::CalcTextSize(txt.c_str()).x;
    if (length > max_length) {
        size_t new_len = size_t(float(txt.length()) * max_length / length);
        return txt.substr(0, new_len) + "...";
    }
    return txt;
}

static void draw_settings(Viewer& viewer, const PrintSettings& settings)
{
    if (!settings.has_data())
        return;

    ImGui::SeparatorText(_u8L("Settings").c_str());

    if (ImGui::BeginTable("Settings", 2)) {
        
        if (!settings.printer.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", _u8L("Printer").c_str());
            ImGui::TableSetColumnIndex(1);
            std::string str = trim_text_if_needed(settings.printer);
            ImGui::Text("%s", str.c_str());
            if (ImGui::IsItemHovered() && str != settings.printer)
                Imgui::tooltip(settings.printer);
        }
      
        if (!settings.print.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", _u8L("Print").c_str());
            ImGui::TableSetColumnIndex(1);
            std::string str = trim_text_if_needed(settings.print);
            ImGui::Text("%s", str.c_str());
            if (ImGui::IsItemHovered() && str != settings.print)
                Imgui::tooltip(settings.print);
        }

        if (!settings.filament.empty()) {
            const std::vector<uint8_t>& used_extruders_ids = viewer.used_extruders_ids();
            for (uint8_t extruder_id : used_extruders_ids) {
                if (extruder_id < static_cast<unsigned char>(settings.filament.size()) && !settings.filament[extruder_id].empty()) {
                    std::string txt = _u8L("Filament");
                    if (viewer.used_extruders_count() > 1)
                        txt += " " + std::to_string(extruder_id + 1);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", txt.c_str());
                    ImGui::TableSetColumnIndex(1);
                    std::string str = trim_text_if_needed(settings.filament[extruder_id]);
                    ImGui::Text("%s", str.c_str());
                    if (ImGui::IsItemHovered() && str != settings.filament[extruder_id])
                        Imgui::tooltip(settings.filament[extruder_id]);
                }
            }
        }

        ImGui::EndTable();
    }
}

static bool draw_estimated_times(Viewer& viewer, Imgui::DoubleSlider::RequestExtraFramesCallback cb = nullptr)
{
    TimeModes time_modes = viewer.time_modes();
    std::vector<std::string> time_modes_str;
    time_modes_str.reserve(time_modes.size());
    for (TimeMode mode : time_modes) {
        time_modes_str.push_back(std::string(to_string(mode)).c_str());
    }

    TimeMode curr_mode = viewer.time_mode();
    std::string curr_mode_str = std::string(to_string(curr_mode));
    int curr_mode_id = int(curr_mode);

    ImGui::SeparatorText(_u8L("Estimated printing times").c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", _u8L("Mode").c_str());
    ImGui::SameLine();

    ImGui::PushItemWidth(-1.0f);
    if (ImGui::BeginCombo("##TimeMode", curr_mode_str.c_str(), ImGuiComboFlags_HeightLargest)) {
        for (int i = 0; i < int(time_modes_str.size()); ++i) {
            if (ImGui::Selectable(time_modes_str[i].c_str(), i == curr_mode_id)) {
                if (i != curr_mode_id)
                    viewer.set_time_mode(TimeMode(i));
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::BeginGroup();
    if (ImGui::BeginTable("TimeEstimates", 2)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", _u8L("First layer").c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", format_time_dhms(viewer.layers_estimated_times().front()).c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", _u8L("Total").c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", format_time_dhms_short(viewer.estimated_time()).c_str());

        ImGui::EndTable();
    }
    ImGui::EndGroup();

    bool ret = ImGui::Button((_u8L("View more") + "...").c_str(), {-1.0f, ImGui::GetCurrentWindow()->DC.CurrLineSize.y});
    if (ret) {
        // force dimmed background without animation
        Imgui::disable_background_fadeout_animation();
        // extra frame to allow imgui to properly size the popup window
        if (cb != nullptr)
            cb(1);
    }
    return ret;
}

static void draw_popup_estimated_times(const char* popup_title, Viewer& viewer)
{
    assert(popup_title != nullptr);

    static TimeMode mode = TimeMode::Normal;

    TimeMode curr_mode = viewer.time_mode();
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);

    bool open = true;

    Imgui::UnifiedWindowStyle unified_window_style;
    unified_window_style.push();
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, { 0.5f, 0.5f });

    ImGuiWindowFlags windows_flag = ImGuiWindowFlags_AlwaysAutoResize
                                  | ImGuiWindowFlags_NoCollapse
                                  | ImGuiWindowFlags_NoMove
                                  | ImGuiWindowFlags_NoResize
                                  | ImGuiWindowFlags_NoScrollbar
                                  | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::OpenPopup(popup_title);
    if (ImGui::BeginPopupModal(popup_title, &open, windows_flag)) {
        const TimeModes time_modes = viewer.time_modes();
        std::vector<std::string> time_modes_str;
        time_modes_str.reserve(time_modes.size());
        for (TimeMode m : time_modes) {
            time_modes_str.push_back(std::string(to_string(m)));
        }

        std::string mode_str = std::string(to_string(mode));
        int mode_id = int(mode);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", _u8L("Mode").c_str());
        ImGui::SameLine();

        ImGui::PushItemWidth(-1.0f);
        if (ImGui::BeginCombo("##TimeMode", mode_str.c_str(), ImGuiComboFlags_HeightLargest)) {
            for (int i = 0; i < int(time_modes_str.size()); ++i) {
                if (ImGui::Selectable(time_modes_str[i].c_str(), i == mode_id)) {
                    if (i != mode_id)
                        mode = TimeMode(i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        viewer.set_time_mode(mode);

        float total_time = viewer.estimated_time();
        float inv_total_time = 100.0f / total_time;

        ImGui::Separator();

        if (ImGui::BeginTable("Time_Estimates_Total", 2)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", _u8L("Total estimated time").c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", format_time_dhms(total_time).c_str());

            ImGui::EndTable();
        }

        ImGui::SeparatorText(_u8L("Features").c_str());

        if (ImGui::BeginTable("Time_Estimates_Features", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner)) {

            // prevent header highlight while hovering or clicking on it
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));

            ImGui::TableSetupColumn("");
            ImGui::TableSetupColumn(_u8L("Feature").c_str());
            ImGui::TableSetupColumn(_u8L("Time").c_str());
            ImGui::TableSetupColumn(_u8L("Percentage").c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGui::PopStyleColor(2);

            GCodeExtrusionRoles roles = viewer.extrusion_roles();

            struct Item
            {
                std::string role_str;
                ColorRGB color;
                float time{ 0.0f };
                float percentage{ 0.0f };
            };

            std::vector<Item> items;
            items.reserve(roles.size());
            for (GCodeExtrusionRole role : roles) {
                float time = viewer.extrusion_role_estimated_time(role);
                const ColorRGB& color = viewer.extrusion_role_color(role);
                items.emplace_back() = { to_string(role), color, time, time * inv_total_time };
            }

            const OptionTypes& options = viewer.options();
            for (const OptionType& option : options) {
                float time = viewer.option_estimated_time(option);
                if (time > 0.0f) {
                    items.emplace_back() = {
                        to_string(option),
                        viewer.option_color(option),
                        time, time * inv_total_time };
                }
            }

            std::sort(items.begin(), items.end(), [](const Item& i1, const Item& i2) { return i1.time > i2.time; });

            for (const Item& item : items) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(item.color));
                ImGui::Dummy(icon_size);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", item.role_str.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", format_time_dhms(item.time).c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.1f%%", item.percentage);
            }

            ImGui::EndTable();
        }

        ImGui::SeparatorText(_u8L("Layers").c_str());

        if (ImGui::BeginTable("Time_Estimates_Layers", 3,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner | ImGuiTableFlags_ScrollY, { -1.0f, 0.333f * ImGui::GetMainViewport()->Size.y })) {

            // prevent header highlight while hovering or clicking on it
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));

            ImGui::TableSetupColumn(_u8L("Layer").c_str());
            ImGui::TableSetupColumn(_u8L("Time").c_str());
            ImGui::TableSetupColumn(_u8L("Overall time").c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGui::PopStyleColor(2);

            std::vector<float> layers_times = viewer.layers_estimated_times();

            float partial_time = 0.0f;
            for (size_t i = 0; i < layers_times.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s #%lu", _u8L("Layer").c_str(), i + 1);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", format_time_dhms(layers_times[i]).c_str());
                partial_time += layers_times[i];
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", format_time_dhms(partial_time).c_str());
            }

            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::NewLine();
        float btn_width = 50.0f;
        ImGui::SameLine(ImGui::GetCurrentWindow()->Size.x - ImGui::GetStyle().WindowPadding.x - btn_width);
        if (ImGui::Button(_u8L("Close").c_str(), { btn_width, 0.0f }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    unified_window_style.pop();

    // restore original mode
    viewer.set_time_mode(curr_mode);
}

void legend(Viewer& viewer, WrapperImpl& wrapper, bool settings_visible, const PrintSettings& settings,
    const LegendCallbacks& cbs)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    Imgui::ScopedGroup group((std::string(window->Name) + "Legend").c_str());

    // show view type selector
    legend_view_type_selector(viewer, wrapper, cbs.cb_view_type_changed, -1.0f);
    // shown items list
    draw_type_items(viewer, wrapper, cbs);
    // show options
    draw_options(viewer, wrapper, cbs.cb_request_extra_frame);
    ViewType type = viewer.view_type();
    if (settings_visible && (type == ViewType::FeatureType || type == ViewType::Tool)) {
        // show producer
        draw_producer(wrapper.producer());
        // show settings
        draw_settings(viewer, settings);
    }
    // show estimated times
    bool estimated_times = false;
    estimated_times |= type == ViewType::FeatureType;
    estimated_times |= type == ViewType::LayerTimeLinear;
    estimated_times |= type == ViewType::LayerTimeLogarithmic;
    estimated_times |= type == ViewType::ColorPrint && has_gcode_events_to_show(viewer);
    if (estimated_times) {
        std::string popup_title = _u8L("Estimated printing times");
        bool popup_open = ImGui::IsPopupOpen(popup_title.c_str());
        if (draw_estimated_times(viewer, cbs.cb_request_extra_frame) || popup_open)
            draw_popup_estimated_times(popup_title.c_str(), viewer);
    }
}

struct CoarseItem
{
    ColorRGB color;
    std::string text;
    CoarseItem(const ColorRGB& color, const std::string& text) : color(color), text(text) {}
};

using CoarseItems = std::vector<CoarseItem>;

static CoarseItems collect_feature_type_coarse_items(Viewer& viewer)
{
    GCodeExtrusionRoles roles = viewer.extrusion_roles();
    CoarseItems ret;
    ret.reserve(roles.size());
    for (GCodeExtrusionRole role : roles) {
        ret.emplace_back(viewer.extrusion_role_color(role), to_string(role));
    }
    return ret;
}

static CoarseItems collect_color_range_coarse_items(Viewer& viewer, WrapperImpl& wrapper)
{
    ViewType type = viewer.view_type();
    const ColorRange& range = viewer.color_range(type);
    std::vector<float> values = range.values();

    CoarseItems ret;
    ret.reserve(values.size());
    for (auto it = values.rbegin(); it != values.rend(); ++it) {
        std::string txt;
        switch (type)
        {
        case ViewType::Height:
        case ViewType::Width:
        {
            txt = convert_and_format_units(*it, UnitsType::Millimeters,
                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches, 3, true);
            break;
        }
        case ViewType::Speed:
        case ViewType::ActualSpeed:
        {
            txt = convert_and_format_units(*it, UnitsType::MillimetersPerSecond,
                (wrapper.units() == UnitsSystem::SI) ? UnitsType::MillimetersPerSecond : UnitsType::InchesPerSecond, 1, true);
            break;
        }
        case ViewType::VolumetricFlowRate:
        case ViewType::ActualVolumetricFlowRate:
        {
            unsigned int decimals = (wrapper.units() == UnitsSystem::SI) ? 3 : 6;
            txt = convert_and_format_units(*it, UnitsType::MillimetersCubePerSecond,
                (wrapper.units() == UnitsSystem::SI) ? UnitsType::MillimetersCubePerSecond : UnitsType::InchesCubePerSecond, decimals, true);
            break;
        }
        case ViewType::FanSpeed:
        {
            txt = format("%.0f%%", *it);
            break;
        }
        case ViewType::Temperature:
        {
            txt = convert_and_format_units(*it, UnitsType::Celsius,
                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Celsius : UnitsType::Farhenheit, 0, true);
            break;
        }
        case ViewType::LayerTimeLinear:
        case ViewType::LayerTimeLogarithmic:
        {
            txt = format_time_dhms(*it);
            break;
        }
        default:
        {
            txt = "Error";
            break;
        }
        }
        ret.emplace_back(range.color_at(*it), txt);
    }
    return ret;
}

static CoarseItems collect_tool_coarse_items(Viewer& viewer)
{
    const std::vector<uint8_t>& used_extruders_ids = viewer.used_extruders_ids();
    const Palette& tool_colors = viewer.tool_colors();

    CoarseItems ret;
    ret.reserve(used_extruders_ids.size());
    for (size_t i = 0; i < used_extruders_ids.size(); ++i) {
        uint8_t extruder_id = used_extruders_ids[i];
        ret.emplace_back(tool_colors[extruder_id], format("%s %d", _u8L("Extruder").c_str(), 1 + extruder_id));
    }
    return ret;
}

static CoarseItems collect_color_print_coarse_items(Viewer& viewer, WrapperImpl& wrapper)
{
    const Palette& color_print_colors = viewer.color_print_colors();
    std::vector<uint8_t> used_extruders_ids = viewer.used_extruders_ids();
    uint8_t extruders_count = viewer.extruders_count();

    CoarseItems ret;
    for (uint8_t id : used_extruders_ids) {
        const ColorPrints& extr_color_prints = viewer.extruder_color_prints(id);
        if (extr_color_prints.size() == 1) {
            std::string txt;
            if (extruders_count > 1)
                txt = format("%s %d %s", _u8L("Extruder").c_str(), 1 + id, _u8L("default color"));
            else
                txt = format("%s", _u8L("Default color"));
            ret.emplace_back(color_print_colors[id], txt);
        }
        else {
            size_t counter = 0;
            for (auto it = extr_color_prints.rbegin(); it != extr_color_prints.rend(); ++it) {
                std::string txt;

                if (counter == 0) {
                    if (extruders_count == 1) {
                        txt = format("%s %s", _u8L("above").c_str(),
                            convert_and_format_units(viewer.layer_z(it->layer_id), UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches,
                                2).c_str());
                    }
                    else {
                        txt = format("%s %d %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                            _u8L("above").c_str(), convert_and_format_units(viewer.layer_z(it->layer_id), UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches, 2).c_str());
                  }
                }
                else if (counter == extr_color_prints.size() - 1) {
                    if (extruders_count == 1) {
                        txt = format("%s %s", _u8L("up to").c_str(),
                            convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1), UnitsType::Millimeters,
                                (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches, 2).c_str());
                    }
                    else {
                        txt = format("%s %d %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                            _u8L("up to").c_str(), convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                                UnitsType::Millimeters, (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches, 2).c_str());
                    }
                }
                else {
                    if (extruders_count == 1) {
                        UnitsType length_units = (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches;
                        std::string txt_from = convert_and_format_units(viewer.layer_z(it->layer_id),
                          UnitsType::Millimeters, length_units, 2, false);
                        std::string txt_to = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                          UnitsType::Millimeters, length_units, 2);
                        txt = format("%s %s %s %s", _u8L("from").c_str(), txt_from.c_str(),
                            _u8L("to").c_str(), txt_to.c_str());
                    }
                    else {
                        UnitsType length_units = (wrapper.units() == UnitsSystem::SI) ? UnitsType::Millimeters : UnitsType::Inches;
                        std::string txt_from = convert_and_format_units(viewer.layer_z(it->layer_id),
                          UnitsType::Millimeters, length_units, 2, false);
                        std::string txt_to = convert_and_format_units(viewer.layer_z(std::prev(it)->layer_id - 1),
                          UnitsType::Millimeters, length_units, 2);
                        txt = format("%s %d %s %s %s %s", _u8L("Extruder").c_str(), 1 + it->extruder_id,
                            _u8L("from").c_str(), txt_from.c_str(), _u8L("to").c_str(), txt_to.c_str());
                    }
                }
                ++counter;

                ret.emplace_back(color_print_colors[it->color_id % color_print_colors.size()], txt);
            }
        }
    }
    return ret;
}

static CoarseItems collect_coarse_items(Viewer& viewer, WrapperImpl& wrapper)
{
    ViewType type = viewer.view_type();
    switch (type)
    {
    case ViewType::FeatureType:              { return collect_feature_type_coarse_items(viewer); }
    case ViewType::Height:
    case ViewType::Width:
    case ViewType::ActualSpeed:
    case ViewType::Speed:
    case ViewType::FanSpeed:
    case ViewType::Temperature:
    case ViewType::VolumetricFlowRate:
    case ViewType::ActualVolumetricFlowRate:
    case ViewType::LayerTimeLinear:
    case ViewType::LayerTimeLogarithmic:     { return collect_color_range_coarse_items(viewer, wrapper); }
    case ViewType::Tool:                     { return collect_tool_coarse_items(viewer); }
    case ViewType::ColorPrint:               { return collect_color_print_coarse_items(viewer, wrapper); }
    default:                                 { break; }
    }
    return CoarseItems();
}

static void draw_coarse_item(CoarseItem& item, const ImVec2& icon_size, float cell_height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::RenderFrame(pos + ImVec2(1.0f, 1.0f), pos + icon_size, Imgui::to_ImU32(item.color), false, 3.0f);
    ImGui::Dummy({ icon_size.x, cell_height });
    ImGui::SameLine();
    ImGui::Text("%s", item.text.c_str());
}

static void draw_coarse_items(Viewer& viewer, WrapperImpl& wrapper)
{
    CoarseItems items = collect_coarse_items(viewer, wrapper);
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 icon_size(line_height, line_height);
    float cell_height = 2.0f * line_height;

    if (ImGui::BeginTable("LegendItems", 2)) {
        for (size_t i = 0; i < items.size(); i += 2) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            draw_coarse_item(items[i], icon_size, cell_height);
            if (i + 1 < items.size()) {
                ImGui::TableSetColumnIndex(1);
                draw_coarse_item(items[i + 1], icon_size, cell_height);
            }
        }
        ImGui::EndTable();
    }
}

void legend_coarse(Viewer& viewer, WrapperImpl& wrapper)
{
    draw_coarse_items(viewer, wrapper);
}

} // namespace Slic3r::App::LibvgcodeWrapper
