#include "TestRenderModule.hpp"
#include "imgui/imgui.h"
#include <iostream>
namespace Slic3r::App {

void TestRenderModule::render_scene() {}

void TestRenderModule::render_imgui()
{
    ImGui::Begin("My Win");
    ImGui::Text("Hello there");
    ImGui::InputText("Text", m_text_buffer, BUF_SIZE);
    if (ImGui::Button("Press me")) {
        // pressed
    }
    if (ImGui::BeginPopupContextItem("MyWinPopup")) {
        ImGui::Text("Item 1");
        ImGui::EndPopup();
    }
    ImGui::End();
}

void TestRenderModule::on_scene_mouse_event(const Platform::MouseEvent &e)
{
    std::cout <<  "MouseEvent type: " << uint32_t(e.get_type()) << "\n";
}

void TestRenderModule::on_scene_keyboard_event(const Platform::KeyboardEvent &e)
{
    std::cout <<  "KeyboardEvent type: " << uint32_t(e.get_type()) << "\n";
}

}