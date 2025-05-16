#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/Assert.hpp"
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Yoga {

Circle::Circle() : Rectangle() { set_aspect_ratio(1.f); }

void Circle::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    ImRect rect(to_im(pos), to_im(pos + size));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ASSERT(draw_list);

    ImColor fill_color;
    if (!enabled()) {
        fill_color = IM_COL32_DISABLE;
    }
    else {
        fill_color = fill();
    }

    draw_list->AddCircleFilled(rect.GetCenter(), 0.5f*rect.GetHeight(), fill_color, 36);
    if (border_width() > 0) {
        draw_list->AddCircle(rect.GetCenter(), 0.5f * rect.GetHeight(), border_color(), 36, border_width());
    }

    render_item_end(pos, size);
}

} // namespace Slic3r::App::Yoga
