#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Plater {

void History::render(ImVec2 pos, ImVec2 size)
{
    ImGui::Text("Actions History");
}

}// Slic3r::App::Plater namespace
