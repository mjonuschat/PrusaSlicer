#include "TestRenderModule.hpp"
#include "imgui/imgui.h"
#include <iostream>
#include <chrono>
#include <fmt/chrono.h>
#include <libslic3r/Geometry.hpp>

#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/ShaderManager.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"

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

std::chrono::duration<double, std::milli> get_delta()
{
    static auto last = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = now - last;
    last = now;
    return delta;
}

TestRenderModule::TestRenderModule()
{
    memset(m_text_buffer, 0, sizeof(m_text_buffer));
}

void TestRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);

    SPDLOG_TRACE("TestRenderModule() 1");
    Render::GeometryBuilder<Render::VertexP3> geometry;
    geometry
        .add_vertex({{1, 1, 0}})
        .add_vertex({{-1, 1, 0}})
        .add_vertex({{0, -1, 0}})
        .add_triangle_indices(0, 1, 2);
    m_geometry = geometry.build(*m_device);

    Render::GeometryBuilder<Render::VertexP3T2> geometry2;
    geometry2
        .add_vertex({{1, 1, 0}, {1, 1}})
        .add_vertex({{-1, 1, 0}, {0, 1}})
        .add_vertex({{1, -1, 0}, {1, 0}})
        .add_vertex({{-1, -1, 0}, {0, 0}})
        .add_triangle_indices(0, 1, 2)
        .add_triangle_indices(1, 3, 2);
    m_geometry2 = geometry2.build(*m_device);

    SPDLOG_TRACE("TestRenderModule() 2");
    auto& ctx = Render::Context::instance();
    m_shader = ctx.shader_manager().get_shader("flat");
    m_shader2 = ctx.shader_manager().get_shader("flat_texture");
    SPDLOG_TRACE("TestRenderModule() 3");
    Render::ImageLoadOptions opts;
    opts.gen_mipmaps = true;
    opts.force_power_of_two = true;
    m_tex = Render::Context::instance().texture_manager().get(
        "icons/PrusaSlicer-gcodeviewer-mac_128px.png",
//         "icons/funnel.svg",
        opts
    );
//    m_tex = Render::Context::instance().texture_manager()
//                .create_empty("white", Render::PixelFormat::RGBA8, 16, 16);
}

void TestRenderModule::render_scene()
{
    m_device->load_state();
    auto command_buffer = m_device->create_command_buffer();
    command_buffer->set_clear_values({0.45f, 0.55f, 0.60f, 1.00f});
    command_buffer->clear_buffers(true, true);

    SPDLOG_TRACE("TestRenderModule::render_scene() 1");

    // glViewport(0, 0, m_screen_info.physical_width(), m_screen_info.physical_height());
    command_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));

    SPDLOG_INFO(
        "Setting viewport to {}x{}", m_screen_info.physical_width(), m_screen_info.physical_height()
    );

    Transform3f view = Transform3f::Identity();
    view = view.translate(Vec3f(0, 0, -2));
    SPDLOG_TRACE("TestRenderModule::render_scene() 2");
    command_buffer->bind_shader(*m_shader);
    //m_shader->bind();
    SPDLOG_TRACE("TestRenderModule::render_scene() 3");

    command_buffer->bind_geometry(*m_geometry, *m_shader);
    Matrix4f vm = view.matrix();
    Matrix4f proj = perspective(
        60,
        float(m_screen_info.physical_width()) / float(m_screen_info.physical_height()),
        0.01, 10
    );


    command_buffer->set_blending({Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha});
    command_buffer->set_blending_enabled(true);

    SPDLOG_TRACE("TestRenderModule::render_scene() 4");
    m_shader->set_uniform("view_model_matrix", vm);
    m_shader->set_uniform("projection_matrix", proj);
    m_shader->set_uniform("uniform_color", std::array<float, 4>{1, 0.5, 0, 1});
    SPDLOG_TRACE("TestRenderModule::render_scene() 5");

    command_buffer->draw(Render::PrimitiveType::Triangles, 0, 3);
    SPDLOG_TRACE("TestRenderModule::render_scene() 6");


    Transform3f model = Transform3f::Identity();
    model.translate(Vec3f(-0.5, -0.5, 0));
    model.scale(m_geom2_scale);
    vm = view.matrix() * model.matrix();

    command_buffer->bind_shader(*m_shader2);
    command_buffer->bind_geometry(*m_geometry2, *m_shader2);
    m_shader2->set_uniform("view_model_matrix", vm);
    m_shader2->set_uniform("projection_matrix", proj);
    m_shader2->set_uniform("uniform_texture", 0);

    command_buffer->bind_texture(0, *m_tex);
    command_buffer->draw(Render::PrimitiveType::Triangles, 0, 6);
    command_buffer->unbind_texture(0, *m_tex);
    SPDLOG_TRACE("TestRenderModule::render_scene() 8");
    command_buffer->submit();
}

void TestRenderModule::render_imgui()
{
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

        ImGui::Text("%s", fmt::format("Delta: {}", get_delta()).c_str());
    }
    ImGui::End();
}

void TestRenderModule::on_scene_mouse_event(const Platform::MouseEvent &e)
{
    if (e.get_type() == Platform::MouseEvent::Type::Move) {
        float dx = 2 * float(e.get_x()) / float(m_screen_info.logical_width()) - 1.0f;
        m_geom2_scale = std::pow(2.0f, dx * 5);
        SPDLOG_INFO("Geom scale: {}  dx: {}", m_geom2_scale, dx);
    }
}

void TestRenderModule::on_scene_keyboard_event(const Platform::KeyboardEvent &e)
{
    std::cout <<  "KeyboardEvent type: " << uint32_t(e.get_type()) << "\n";
}

}
