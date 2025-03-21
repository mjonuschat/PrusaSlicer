#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App {

void CubeView::render(ImVec2 pos, ImVec2 size)
{
    // !!! temporary code. Needed tobe changed for real view cube
    Imgui::icon_button(ImGui::CubeViewIcon, ImVec2(70.f, 70.f));
}

}// Slic3r::App namespace
