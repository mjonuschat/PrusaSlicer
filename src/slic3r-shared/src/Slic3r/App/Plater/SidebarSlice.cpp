#include "Slic3r/App/Plater/SidebarSlice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Plater {

void SidebarSlice::render(ImVec2 pos, ImVec2 size)
{
//    ImGui::SetWindowFontScale(1.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    ImGui::Button("Slice", ImVec2(size.x, 45.f));
    ImGui::PopStyleColor();

    ImGui::SetWindowFontScale(1.f);
}

}// Slic3r::App::Plater namespace
