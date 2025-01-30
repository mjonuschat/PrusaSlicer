#include "Slic3r/App/Preview/DoubleSliderForGCode.hpp"

namespace Slic3r::App::Preview {

static constexpr float SLIDER_GCODE_HEIGHT = 40.0f;

void DoubleSliderForGcode::render(float scale_factor/* = 0.1f*/, float offset/* = 0.f*/)
{
    if (!m_ctrl.is_shown())
        return;

    m_scale = scale_factor;

    ImGuiViewport& viewport = *ImGui::GetMainViewport();
    float height = SLIDER_GCODE_HEIGHT * m_scale;
    ImVec2 size(viewport.Size.x - 2.0f * offset, height);
    ImVec2 pos((viewport.Size.x - size.x) * 0.5f, viewport.Size.y - height);

    m_ctrl.init(pos, size, m_scale);
    if (m_ctrl.render())
        process_thumb_move();
}

} // namespace Slic3r::App::Preview
