#include "TestRenderModule.hpp"
#include "imgui/imgui.h"
#include <iostream>
#include <libslic3r/Geometry.hpp>

#include "Render/commonGL.hpp"
#include "Render/Context.hpp"
#include "Render/ShaderManager.hpp"

namespace Slic3r::App {

Matrix4f frustum(float left, float right, float bottom, float top, float near_z, float far_z)
{
    const float inv_dx = 1.0f / (right - left);
    const float inv_dy = 1.0f / (top - bottom);
    const float inv_dz = 1.0f / (far_z - near_z);
    Matrix4f ret;
    ret << 2.0f * near_z * inv_dx,                    0.0,    (left + right) * inv_dx,                             0.0,
                              0.0, 2.0f * near_z * inv_dy,    (bottom + top) * inv_dy,                             0.0,
                              0.0,                    0.0, -(near_z + far_z) * inv_dz, -2.0f * near_z * far_z * inv_dz,
                              0.0,                    0.0,                       -1.0,                             0.0;
    return ret;
}

Matrix4f perspective(float fovy, float aspect, float near_z, float far_z)
{
    const float f = 1.0f / std::tan(Geometry::deg2rad(fovy / 2));
    const float dist_z = near_z - far_z;
    Matrix4f ret;
    ret <<
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far_z + near_z) / dist_z, 2 * far_z * near_z / dist_z,
        0, 0, -1, 0;
    return ret;
}

TestRenderModule::TestRenderModule()
{
    SPDLOG_INFO("TestRenderModule() 1");
    std::cerr << "X\n";
    memset(m_text_buffer, 0, sizeof(m_text_buffer));
    m_geometry
        .add_vertex({{1, 1, -2}})
        .add_vertex({{-1, 1, -2}})
        .add_vertex({{0, -1, -2}})
        .add_triangle_indices(0, 1, 2)
        .upload();

    SPDLOG_INFO("TestRenderModule() 2");
    auto& ctx = Render::Context::instance();
    m_shader = ctx.shader_manager().get_shader("flat");
    SPDLOG_INFO("TestRenderModule() 3");
}

void TestRenderModule::render_scene()
{
    SPDLOG_INFO("TestRenderModule::render_scene() 1");
    glViewport(0, 0, m_screen_w, m_screen_h);
    Transform3f view = Transform3f::Identity();
    //view = view.translate(Vec3f(0, 0, -2));
    SPDLOG_INFO("TestRenderModule::render_scene() 2");
    m_shader->bind();
    SPDLOG_INFO("TestRenderModule::render_scene() 3");

    m_geometry.bind(*m_shader);
    Matrix4f vm = view.matrix();
    const float scale = 1.0f / std::max(m_screen_w, m_screen_h);
    //Matrix4f proj = frustum(-m_screen_w * scale, m_screen_w * scale, -m_screen_h * scale, m_screen_h * scale, 0.01, 10);
    //Matrix4f proj = frustum(-1, 1, -1, 1 * scale, 0.01, 10);
    Matrix4f  proj = perspective(60, float(m_screen_w) / float(m_screen_h), 0.01, 10);
    glEnable(GL_BLEND);
    SPDLOG_INFO("TestRenderModule::render_scene() 4");
    m_shader->set_uniform("view_model_matrix", vm);
    m_shader->set_uniform("projection_matrix", proj);
    m_shader->set_uniform("uniform_color", std::array<float, 4>{1, 0.5, 0, 1});
    SPDLOG_INFO("TestRenderModule::render_scene() 5");

    m_geometry.draw(GL_TRIANGLES, 0, 3);
    SPDLOG_INFO("TestRenderModule::render_scene() 6");

    m_geometry.unbind();
    SPDLOG_INFO("TestRenderModule::render_scene() 7");
    m_shader->unbind();
    SPDLOG_INFO("TestRenderModule::render_scene() 8");


}

void TestRenderModule::render_imgui()
{
    glCheck();

    if (ImGui::Begin("My Win")) {
        ImGui::Text("Hello there");
        ImGui::InputText("Text", m_text_buffer, BUF_SIZE);
        if (ImGui::Button("Press me")) {
            // pressed
        }
        if (ImGui::BeginPopupContextItem("MyWinPopup")) {
            ImGui::Text("Item 1");
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    glCheck();

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