#pragma once

#include <Slic3r/Biz/libpgcode/LineView.hpp>
#include "Slic3r/App/Yoga/Window.hpp"

#include <string_view>
#include <optional>
#include <cstdint>
#include <memory>

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


    void set_gcode(std::shared_ptr<const Biz::libpgcode::LineView>& gcode) { m_gcode = gcode; }
    bool has_data() const { return (m_gcode && !m_gcode->empty()); }

    void reset() { m_gcode = nullptr; }

    Line line_at(uint32_t id) const;

    void resize_range(Range& range, uint32_t lines_count, uint32_t curr_line_id) const;

private:
    std::shared_ptr<const Biz::libpgcode::LineView> m_gcode;
};


/** @brief ImGui widget to show the gcode lines as list of strings.
 *
 * @param data The data to show
 * @param curr_line_id The current line id
 * @param clip_text Whether or not to clip the text
 */
class GCodeWindow : public Yoga::Window {
public:
    GCodeWindow();

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

    void set_data(GCodeWindowData* data);
    void set_curr_line_id(uint32_t curr_line_id);
    void set_clip_text(bool clip_text);

private:
    GCodeWindowData* m_data = nullptr;
    uint32_t m_curr_line_id = 0;
    bool m_clip_text = false;
};

} // namespace Slic3r::App::Preview
