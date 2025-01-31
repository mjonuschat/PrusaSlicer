#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <cstdint>

namespace Slic3r::App::Preview {

class GCodeWindowData
{
public:
    struct Line
    {
        std::string_view command;
        std::string_view parameters;
        std::string_view comment;

        bool empty() const { return command.empty() && parameters.empty() && comment.empty(); }
    };

    struct Range
    {
        std::optional<uint32_t> min;
        std::optional<uint32_t> max;
        bool empty() const { return !min.has_value() || !max.has_value(); }
        bool contains(const Range& other) const {
            return !this->empty() && !other.empty() && *this->min <= *other.min && *this->max >= *other.max;
        }
        uint32_t size() const { return empty() ? 0 : *this->max - *this->min + 1; }
        void reset() { min = std::nullopt; max = std::nullopt; }
    };

    bool is_visible() const { return m_visible; }
    void set_visible(bool visible) { m_visible = visible; }
    void toggle_visible() { m_visible = !m_visible; }

    void set_gcode(std::vector<std::string>&& gcode) { m_gcode = std::move(gcode); }
    bool has_data() const { return !m_gcode.empty(); }

    void reset() { m_gcode.clear(); }

    const Line& line_at(uint32_t id) const;

    void resize_range(Range& range, uint32_t lines_count, uint32_t curr_line_id) const;

private:
    bool m_visible{ true };
    std::vector<std::string> m_gcode;
};

/** @brief ImGui widget to show the gcode lines.
 *
 * @param data The data to show
 * @param curr_line_id The current line id 
 */
void gcode_window(const GCodeWindowData& data, uint32_t curr_line_id);

} // namespace Slic3r::App::Preview
