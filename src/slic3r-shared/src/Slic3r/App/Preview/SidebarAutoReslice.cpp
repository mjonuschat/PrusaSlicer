#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Preview {

void SidebarAutoReslice::render(ImVec2 pos, ImVec2 size)
{
    static bool check{ true };
    ImGui::Checkbox("Auto re-slice", &check);
}

}// Slic3r::App::Preview namespace
