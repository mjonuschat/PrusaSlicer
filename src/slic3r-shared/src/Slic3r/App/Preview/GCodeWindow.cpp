#include "Slic3r/App/Preview/GCodeWindow.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Preview {

static const ImVec4 COMMAND_COLOR    = { 0.8f, 0.8f, 0.0f, 1.0f };
static const ImVec4 PARAMETERS_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };
static const ImVec4 COMMENT_COLOR    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const ImVec4 ELLIPSIS_COLOR   = { 0.0f, 0.7f, 0.0f, 1.0f };

const GCodeWindowData::Line& GCodeWindowData::line_at(uint32_t id) const
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

static std::string reduce_string(const std::string& src, size_t current_length)
{
    static const size_t LENGTH_THRESHOLD = 60;
    return (current_length + src.length() > LENGTH_THRESHOLD) ? src.substr(0, LENGTH_THRESHOLD - current_length) : src;
}

void gcode_window(const GCodeWindowData& data, uint32_t curr_line_id)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    Imgui::ScopedGroup group((std::string(window->Name) + "GCodeWindow").c_str());

    float line_height = ImGui::GetTextLineHeight();
    const ImGuiStyle& style = ImGui::GetStyle();
    float cell_height = line_height + 2.0f * style.CellPadding.y;

    if (!data.has_data()) {
        static std::string msg = _u8L("No data available");
        ImVec2 msg_size = ImGui::CalcTextSize(msg.c_str(), nullptr, true) +
            (style.WindowPadding + style.FramePadding) * 2.0f;
        ImVec2 internal_frame_size = ImGui::CalcItemSize({ msg_size.x, -1.0f }, msg_size.x, msg_size.y);
        ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + internal_frame_size);
        ImGui::Dummy(frame_bb.GetSize());
        ImGui::RenderText(frame_bb.Min + (frame_bb.GetSize() - ImGui::CalcTextSize(msg.c_str())) * 0.5f, msg.c_str());
    }
    else {
        const uint32_t visible_lines_count = uint32_t(window->ContentRegionRect.GetSize().y / cell_height);
        if (visible_lines_count == 0)
            return;

        // visible range
        GCodeWindowData::Range visible_range;
        data.resize_range(visible_range, visible_lines_count, curr_line_id);

        float top_padding_y = 0.5f * (window->ContentRegionRect.GetSize().y - float(visible_lines_count) * cell_height);
        window->DC.CursorPos.y += top_padding_y;

        if (ImGui::BeginTable("GCodeLines", 2)) {
            float max_id_width = ImGui::CalcTextSize(std::to_string(*visible_range.max).c_str()).x;
            for (uint32_t id = *visible_range.min; id <= *visible_range.max; ++id) {
                // rect around the current selected line
                if (id == curr_line_id) {
                    ImVec2 pos(window->DC.CursorStartPos.x - 0.5f * style.WindowPadding.x, window->DC.CursorPos.y);
                    ImGui::GetWindowDrawList()->AddRect({ pos.x, pos.y },
                        { pos.x + ImGui::GetWindowWidth() - style.WindowPadding.x, pos.y + line_height + style.CellPadding.y },
                        ImGui::GetColorU32(ImGuiCol_Separator));
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                // spacer to right align text
                float id_width = ImGui::CalcTextSize(std::to_string(id).c_str()).x;
                if (id_width < max_id_width) {
                    ImGui::Dummy({ max_id_width - id_width, line_height });
                    ImGui::SameLine(0.0f, 0.0f);
                }
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator), "%u", id);

                const GCodeWindowData::Line& line = data.line_at(id - 1);
                if (!line.empty()) {
                    ImGui::TableSetColumnIndex(1);

                    size_t current_length = 0;
                    bool show_ellipsis = false;
                    if (!line.command.empty()) {
                        std::string_view str = reduce_string(line.command, current_length);
                        ImGui::TextColored(COMMAND_COLOR, "%s", std::string(str).c_str());
                        current_length += str.length() + 1;
                        show_ellipsis = str != line.command;
                    }
                    if (!show_ellipsis && !line.parameters.empty()) {
                        if (current_length > 0)
                            ImGui::SameLine(0.0f, 0.0f);
                        std::string_view str = reduce_string(line.parameters, current_length);
                        ImGui::TextColored(PARAMETERS_COLOR, "%s", std::string(str).c_str());
                        current_length += str.length() + 1;
                        show_ellipsis = str != line.parameters;
                    }
                    if (!show_ellipsis && !line.comment.empty()) {
                        if (current_length > 0)
                            ImGui::SameLine(0.0f, 0.0f);
                        std::string_view str = reduce_string(line.comment, current_length);
                        ImGui::TextColored(COMMENT_COLOR, "%s", std::string(str).c_str());
                        show_ellipsis = str != line.comment;
                    }
                    if (show_ellipsis) {
                        ImGui::SameLine(0.0f, 0.0f);
                        ImGui::TextColored(ELLIPSIS_COLOR, "...");
                    }
                }
            }

            ImGui::EndTable();
        }
    }
}

} // namespace Slic3r::App::Preview
