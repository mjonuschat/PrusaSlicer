#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App {

void SidebarPrint::render(ImVec2 pos, ImVec2 size)
{
    ImGui::Button("Balanced settings", ImVec2(size.x, 0.f));
    ImGui::Button("Other button", ImVec2(size.x, 0.f));
}

}// Slic3r::App::Plater namespace
