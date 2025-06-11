#include "Slic3r/App/Preview/GCodeWindow.hpp"
#include "Slic3r/App/Preview/GCodeDisplay.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Preview {

GCodeWindowData::Line GCodeWindowData::line_at(uint32_t id) const
{
    static const Line DUMMY_LINE = Line();
    if (id < uint32_t(m_gcode->size())) {
        std::string_view payload = (*m_gcode)[id];

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
    uint32_t total_lines_count = uint32_t(m_gcode->size());
    DEBUG_ASSERT(total_lines_count > 0);
    uint32_t half_lines_count = lines_count / 2;
    range.min = (curr_line_id > half_lines_count) ? curr_line_id - half_lines_count : 1;
    range.max = *range.min + lines_count - 1;
    if (*range.max >= total_lines_count) {
        range.max = total_lines_count - 1;
        range.min = *range.max - lines_count + 1;
    }
}

GCodeWindow::GCodeWindow(libvgcode::FdmViewer* viewer, GCodeWindowData* data) :
    Yoga::Window("gcode_window")
{
    set_min_size({ 330.f, 0.f });
    set_orientation(Yoga::Orientation::Vertical);

    emplace_back<Yoga::Text>(_u8L("G-code viewer"))
        ->set_font_type(App::Render::ImguiFontType::Bold);

    m_gcode = emplace_back<GCodeDisplay>(viewer, data);
    m_gcode->set_flex_grow(1);
}

void GCodeWindow::set_clip_text(bool clip_text) { m_gcode->set_clip_text(clip_text); }

} // namespace Slic3r::App::Preview
