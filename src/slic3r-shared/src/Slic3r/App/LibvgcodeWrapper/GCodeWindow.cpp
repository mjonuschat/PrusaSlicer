#include "Slic3r/App/LibvgcodeWrapper/GCodeWindow.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::LibvgcodeWrapper {

static ImVec4 id_color() { return { 0.99f, 0.41f, 0.2f, 1.0f }; } // color from SidebarAfterSlice::render()
static ImVec4 cmd_color() { return { 0.8f, 0.8f, 0.0f, 1.0f }; }
static ImVec4 params_color() { return ImGui::GetStyleColorVec4(ImGuiCol_Text); }
static ImVec4 comment_color() { return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled); }
static ImVec4 ellipsis_color() { return { 0.0f, 0.7f, 0.0f, 1.0f }; }
static ImVec4 highlight_bg_color() { return { 0.4f, 0.4f, 0.4f, 1.0f }; }
static ImVec4 no_highlight_bg_color() { return ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg); }

GCodeWindowData::Line GCodeWindowData::line_at(uint32_t id) const
{
    static const Line DUMMY_LINE = Line();
    if (id < uint32_t(m_gcode.size())) {
        std::string_view payload = m_gcode[id];

        size_t pos = payload.find('\n');
        if (pos != payload.npos)
            payload = payload.substr(0, pos);

        Line line;
        pos = payload.find(';');
        if (pos != payload.npos)
            line.comment = payload.substr(pos);
        payload = payload.substr(0, pos);

        pos = payload.find(' ');
        if (pos != payload.npos) {
            line.command = payload.substr(0, pos);
            line.parameters = payload.substr(pos);
        }
        return line;
    }
    else
        return DUMMY_LINE;
}

void GCodeWindowData::resize_range(Range& range, uint32_t lines_count, uint32_t curr_line_id) const
{
    uint32_t total_lines_count = uint32_t(m_gcode.size());
    DEBUG_ASSERT(total_lines_count > 0);
    uint32_t half_lines_count = lines_count / 2;
    range.min = (curr_line_id > half_lines_count) ? curr_line_id - half_lines_count : 1;
    range.max = *range.min + lines_count - 1;
    if (*range.max >= total_lines_count) {
        range.max = total_lines_count - 1;
        range.min = *range.max - lines_count + 1;
    }
}

static std::string_view reduce_string(const std::string_view src, size_t current_length)
{
    static const size_t LENGTH_THRESHOLD = 60;
    return (current_length + src.length() > LENGTH_THRESHOLD) ? src.substr(0, LENGTH_THRESHOLD - current_length) : src;
}

void gcode_window(const GCodeWindowData& data, uint32_t curr_line_id, bool clip_text)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (!data.has_data()) {
        static std::string msg = _u8L("No data available");
        ImVec2 msg_size = ImGui::CalcTextSize(msg.c_str());
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        if (msg_size.x < available_size.x && msg_size.y < available_size.y)
            ImGui::RenderText(window->DC.CursorPos + (available_size - msg_size) * 0.5f, msg.c_str());
    }
    else {
        float line_height = ImGui::GetTextLineHeight();
        const ImGuiStyle& style = ImGui::GetStyle();
        float cell_height = line_height + 2.0f * style.CellPadding.y;
        float available_height = ImGui::GetContentRegionAvail().y;

        const uint32_t visible_lines_count = uint32_t(available_height / cell_height);
        if (visible_lines_count == 0)
            return;

        // visible range
        GCodeWindowData::Range visible_range;
        data.resize_range(visible_range, visible_lines_count, curr_line_id);

        float top_padding_y = available_height - float(visible_lines_count) * cell_height;
        window->DC.CursorPos.y += top_padding_y;

        if (ImGui::BeginTable("GCodeLines", 2, ImGuiTableFlags_SizingFixedFit)) {
            float max_id_width = ImGui::CalcTextSize(std::to_string(*visible_range.max).c_str()).x;
            for (uint32_t id = *visible_range.min; id <= *visible_range.max; ++id) {
                ImGui::TableNextRow();
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                    ImGui::GetColorU32((id == curr_line_id) ? highlight_bg_color() : no_highlight_bg_color()));

                ImGui::TableSetColumnIndex(0);
                // spacer to right align text
                float id_width = ImGui::CalcTextSize(std::to_string(id).c_str()).x;
                if (id_width < max_id_width) {
                    ImGui::Dummy({ max_id_width - id_width, line_height });
                    ImGui::SameLine(0.0f, 0.0f);
                }
                ImGui::TextColored(id_color(), "%u", id);

                const GCodeWindowData::Line& line = data.line_at(id - 1);
                if (!line.empty()) {
                    ImGui::TableSetColumnIndex(1);

                    size_t current_length = 0;
                    bool show_ellipsis = false;
                    if (!line.command.empty()) {
                        std::string_view str = clip_text ? reduce_string(line.command, current_length) : line.command;
                        ImGui::TextColored(cmd_color(), "%s", std::string(str).c_str());
                        current_length += str.length() + 1;
                        if (clip_text) show_ellipsis = str != line.command;
                    }
                    if (!show_ellipsis && !line.parameters.empty()) {
                        if (current_length > 0)
                            ImGui::SameLine(0.0f, 0.0f);
                        std::string_view str = clip_text ? reduce_string(line.parameters, current_length) : line.parameters;
                        ImGui::TextColored(params_color(), "%s", std::string(str).c_str());
                        current_length += str.length() + 1;
                        if (clip_text) show_ellipsis = str != line.parameters;
                    }
                    if (!show_ellipsis && !line.comment.empty()) {
                        if (current_length > 0)
                            ImGui::SameLine(0.0f, 0.0f);
                        std::string_view str = clip_text ? reduce_string(line.comment, current_length) : line.comment;
                        ImGui::TextColored(comment_color(), "%s", std::string(str).c_str());
                        if (clip_text) show_ellipsis = str != line.comment;
                    }
                    if (show_ellipsis) {
                        ImGui::SameLine(0.0f, 0.0f);
                        ImGui::TextColored(ellipsis_color(), "...");
                    }
                }
            }

            ImGui::EndTable();
        }
    }
}

} // namespace Slic3r::App::LibvgcodeWrapper
