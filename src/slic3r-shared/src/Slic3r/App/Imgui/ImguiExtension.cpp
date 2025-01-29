
#include "Slic3r/App/Imgui/DoubleSlider.hpp"

namespace Slic3r::App::Imgui {

void draw_hexagon(const ImVec2& center, float radius, ImU32 col, float start_angle, float rounding)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const float TWO_PI = 2.0f * float(IM_PI);
    float a_min = start_angle;
    float a_max = start_angle + TWO_PI;

    if (rounding <= 0)
        window->DrawList->PathArcTo(center, radius, a_min, a_max, 6);
    else {
        const float a_delta = IM_PI / 4.0f;
        radius -= rounding;

        for (int i = 0; i <= 6; i++) {
            float a = a_min + ((float)i / (float)6) * (a_max - a_min);
            if (a >= TWO_PI)
                a -= TWO_PI;
            ImVec2 pos = ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius);
            window->DrawList->PathArcTo(pos, rounding, a - a_delta, a + a_delta, 5);
        }
    }
    window->DrawList->PathFillConvex(col);
}

} // namespace Slic3r::App::Imgui
