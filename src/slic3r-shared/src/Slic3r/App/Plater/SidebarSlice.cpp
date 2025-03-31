#include "Slic3r/App/Plater/SidebarSlice.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"

namespace Slic3r::App::Plater {

void SidebarSlice::render(Domain::Vec2f pos, Domain::Vec2f size)
{
    ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    if (m_export_allowed) {
        if (ImGui::Button("Slice", ImVec2(size.x()/ 2, 45.f)))
            on_slice();
        ImGui::SameLine();
        if (ImGui::Button("Export", ImVec2(size.x() /2, 45.f)))
            m_export_fn();
    } else {
        if (ImGui::Button("Slice", ImVec2(size.x(), 45.f)))
            on_slice();
    }
    ImGui::PopStyleColor();
    ImGui::PopFont();
}
void SidebarSlice::on_slice()
{
     m_slice_fn();
     m_export_allowed = false;
}

}// Slic3r::App::Plater namespace
