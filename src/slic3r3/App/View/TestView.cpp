//
// Created by Jan Bartipan on 05.04.2024.
//

#include "TestView.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::View {

void TestView::render_imgui()
{
    ImGui::Begin("##TestRenderModule");
    ImGui::Text("Hello from Test Panel");
    ImGui::SliderFloat("Value", &value, 0, 10);

    ImGui::InputText("Text", str, str_capacity);

    if (ImGui::Button("OK")) {

    }

    ImGui::End();

}

} // namespace Slic3r::App::View