#include "Slic3r/App/Preview/DoubleSliderForGCode.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Preview {

static constexpr float SLIDER_GCODE_HEIGHT = 40.0f;

DoubleSliderForGcode::DoubleSliderForGcode()
: Imgui::DoubleSlider::Manager<unsigned int>(std::string("gcode_slider"), Biz::L("Steps"), Yoga::Orientation::Horizontal)
{
    set_min_size({100, SLIDER_GCODE_HEIGHT});
    set_flex_shrink(0);
    m_ctrl->callbacks().value_changed = [this]() { process_thumb_move(); };
    m_ctrl->callbacks().request_extra_frame = [this]() { process_request_extra_frames(); };
}

} // namespace Slic3r::App::Preview
