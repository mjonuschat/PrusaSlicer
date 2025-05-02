#include "Slic3r/App/Yoga/IconButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Yoga {

IconButton::IconButton(wchar_t icon, const std::string& tooltip, Item* parent)
    : AbstractButton(icon, tooltip, parent)
{}

void IconButton::render(Vec2f pos, Vec2f size)
{
    float rounding = GImGui->Style.WindowRounding; // { 4.f };
    ImVec2 button_size = to_im(size);
    ImRect button_bb(to_im(pos), to_im(pos) + button_size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float arrow_h = draw_list->_Data->FontSize * 0.5f;

    ImVec2 arrow_size = ImVec2(1.f, 1.f) * 2.5f * arrow_h;
    ImVec2 arrow_pos = to_im(pos) + button_size - arrow_size;
    ImRect arrow_bb(arrow_pos, arrow_pos + arrow_size);

    // Check if the arrow is clicked or hovered
    bool hovered_arrow = m_has_arrow && ImGui::IsMouseHoveringRect(arrow_bb.Min, arrow_bb.Max);
    bool pressed_arrow = m_has_arrow && hovered_arrow && ImGui::IsMouseClicked(0);

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max) && !hovered_arrow;
    bool pressed = m_enabled && hovered && ImGui::IsMouseClicked(0);

    // Draw button background with custom rounding corner(s)
    draw_list->AddRectFilled(
        button_bb.Min, button_bb.Max, ImGui::GetColorU32(ImGuiCol_WindowBg), rounding,
        ImDrawFlags_RoundCornersAll
        // TODO: resolve rounding rounding_corners
    );
    button_bb.Expand(-rounding);
    if (m_enabled) {
        ImU32 col = ImGui::GetColorU32(
            hovered ? (m_checked ? ImGuiCol_Button : ImGuiCol_ButtonHovered)
                    : (m_checked ? ImGuiCol_ButtonHovered : ImGuiCol_Button)
        );
        draw_list
            ->AddRectFilled(button_bb.Min, button_bb.Max, col, rounding, ImDrawFlags_RoundCornersAll);
    }

    // Render icon in the center of the button
    ImGui::SetCursorScreenPos(button_bb.Min);
    Imgui::icon_image(m_icon, button_bb.GetSize(), !m_enabled);

    if (m_has_arrow) {
        // Draw button background with custom rounding on only one corner
        ImU32 arrow_col = ImGui::GetColorU32(hovered_arrow ? ImGuiCol_ButtonHovered : ImGuiCol_Text);

        // draw arrow
        ImVec2 corner_pos = arrow_bb.GetCenter() + ImVec2(1.f, 1.f) * 0.5f * arrow_h;
        draw_list->AddTriangleFilled(
            corner_pos + ImVec2(0.f, -arrow_h), corner_pos, corner_pos + ImVec2(-arrow_h, 0.f),
            arrow_col
        );
    }

    m_tooltip->set_visible(hovered || hovered_arrow);

    if (pressed) {
        if (m_callbacks.action) {
            m_callbacks.action();
        }
    } else if (pressed_arrow) {
        if (m_callbacks.action_on_arrow) {
            m_callbacks.action_on_arrow();
        }
    } else if (hovered_arrow) {
        if (m_callbacks.action_on_arrow_hovering) {
            m_callbacks.action_on_arrow_hovering();
        }
    }
}

} // namespace Slic3r::App::Yoga
