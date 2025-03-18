#include "Slic3r/App/Preview/SidebarAfterSlice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Preview {

static void add_centered_icon_button(wchar_t icon, const std::string& id)
{
    float h = 1.25f * ImGui::GetTextLineHeight();
    ImVec2 btn_sz(h, h);
    float offsetX = (ImGui::GetColumnWidth() - btn_sz.x) * 0.5 - GImGui->Style.FramePadding.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

    Imgui::icon_button(icon, btn_sz, id);
}

void SidebarAfterSlice::render(ImVec2 pos, ImVec2 size)
{
    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadInnerX;
    if (ImGui::BeginTable("##ObjectListTable", 4, table_flags)) {
        ImGui::TableNextColumn(); add_centered_icon_button(ImGui::SavePrint, "SavePrint");
        ImGui::TableNextColumn(); add_centered_icon_button(ImGui::SavePrintToFlash, "SavePrintToFlash");
        ImGui::TableNextColumn(); add_centered_icon_button(ImGui::SavePrintToLocal, "SavePrintToLocal");
        ImGui::TableNextColumn(); add_centered_icon_button(ImGui::SavePrintAddBookmark, "SavePrintAddBookmark");

        ImGui::EndTable();
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.f);

//    ImGui::SetWindowFontScale(1.5f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.32f, 0.48f, 0.84f, 1.0f));
    ImGui::Button("<", ImVec2(40.f, 45.f));
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    const float next_pos = 40.f + GImGui->Style.ItemSpacing.x;
    ImGui::Button("Print", ImVec2(size.x - next_pos, 45.f));
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.f);
}

}// Slic3r::App::Preview namespace
