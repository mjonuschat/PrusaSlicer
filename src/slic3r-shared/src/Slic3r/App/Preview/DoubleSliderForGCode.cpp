#include "Slic3r/App/Preview/DoubleSliderForGCode.hpp"

namespace Slic3r::App::Preview {

static constexpr float SLIDER_GCODE_HEIGHT = 40.0f;

void DoubleSliderForGcode::render_body(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    m_ctrl.init(to_im(pos), to_im(size), m_scale);
    if (m_ctrl.render()) {
        process_thumb_move();
    }
}

} // namespace Slic3r::App::Preview
