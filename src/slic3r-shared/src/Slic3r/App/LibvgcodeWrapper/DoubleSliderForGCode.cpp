#include "Slic3r/App/LibvgcodeWrapper/DoubleSliderForGCode.hpp"

namespace Slic3r::App::LibvgcodeWrapper {

static constexpr float SLIDER_GCODE_HEIGHT = 40.0f;

void DoubleSliderForGcode::render(const ImVec2& pos, float scale_factor, float offset)
{
    if (!m_ctrl.is_shown())
        return;

    m_scale = scale_factor;

    float h = SLIDER_GCODE_HEIGHT * m_scale;
    ImVec2 size;
    ImVec2 position;
    if (pos.x == -1.0f && pos.y == -1.0f) {
        // temporary hack to allow to render the slider outside Yoga layout
        ImGuiViewport& viewport = *ImGui::GetMainViewport();
        size = { viewport.Size.x - 2.0f * offset, h };
        position = { (viewport.Size.x - size.x) * 0.5f, viewport.Size.y - h };
    }
    else {
        ImVec2 av = ImGui::GetContentRegionAvail();
        ImVec2 cp = ImGui::GetCursorScreenPos();

        size = { av.x - 2.0f * offset , h };
        position = { cp.x + offset, cp.y + 0.5f * (av.y - h) };
    }

    m_ctrl.init(position, size, m_scale);
    if (m_ctrl.render())
        process_thumb_move();
}

} // namespace Slic3r::App::LibvgcodeWrapper
