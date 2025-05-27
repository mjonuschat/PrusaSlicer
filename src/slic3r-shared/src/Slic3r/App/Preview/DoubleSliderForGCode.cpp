#include "Slic3r/App/Preview/DoubleSliderForGCode.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

namespace Slic3r::App::Preview {

static constexpr float SLIDER_GCODE_HEIGHT = 40.0f;

DoubleSliderForGcode::DoubleSliderForGcode()
: Imgui::DoubleSlider::Manager<unsigned int>(std::string("gcode_slider"), L("Steps"), Yoga::Orientation::Horizontal)
{
    m_ctrl->callbacks().value_changed = [this]() { process_thumb_move(); };
}

} // namespace Slic3r::App::Preview
