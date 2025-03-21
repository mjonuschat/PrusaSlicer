#include "Slic3r/App/Plater/SidebarSlice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"

namespace Slic3r::App::Plater {

void SidebarSlice::render(Domain::Vec2f pos, Domain::Vec2f size)
{
    ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    if (ImGui::Button("Slice", ImVec2(size.x(), 45.f)))
        m_slice_fn();
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

}// Slic3r::App::Plater namespace
