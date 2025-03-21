#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Preview {

void SidebarAutoReslice::render(Domain::Vec2f pos, Domain::Vec2f size)
{
    static bool check{ true };
    ImGui::Checkbox("Auto re-slice", &check);
}

}// Slic3r::App::Preview namespace
