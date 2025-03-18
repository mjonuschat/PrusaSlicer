#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App {

static bool PrinterButton(wchar_t icon, float width, const std::string& model, const std::string& name, bool is_toggled)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float rounding = GImGui->Style.WindowRounding;
    ImVec2 button_size(width, 60.f);
    ImRect button_bb(pos, pos + button_size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max);
    bool pressed = hovered && ImGui::IsMouseClicked(0);

    ImU32 col = ImGui::GetColorU32(hovered ? (is_toggled ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : (is_toggled ? ImGuiCol_ButtonActive : ImGuiCol_Button));
    draw_list->AddRectFilled(button_bb.Min, button_bb.Max, col, rounding, ImDrawFlags_RoundCornersAll);
    button_bb.Expand(-10.f);

    // Render icon in the center of the button
    ImGui::SetCursorScreenPos(button_bb.Min);
    Imgui::icon_image(icon, ImVec2(40.f, 40.f));

    button_bb.Min.x += 50.f;
    draw_list->AddText(button_bb.Min, ImGui::GetColorU32(ImGuiCol_Text), (model+ " / "+ name).c_str());

    if (hovered) {
        ImGui::SetCursorScreenPos(button_bb.Max - ImVec2(50.f, 34.f));
        Imgui::icon_button(ImGui::ConfigContainer, ImVec2(), "printer");

        ImGui::SetCursorScreenPos(button_bb.Max - ImVec2(20.f, 34.f));
        Imgui::icon_button(ImGui::PrintIconMarker, ImVec2(), "settings");
    }
    
    return pressed;
}

void SidebarBed::render(ImVec2 pos, ImVec2 size)
{
    ImGui::Text("Bed");
    PrinterButton(ImGui::PrinterNEXT, 240.f, "NEXT", "Elsa", false);
}

}// Slic3r::App namespace
