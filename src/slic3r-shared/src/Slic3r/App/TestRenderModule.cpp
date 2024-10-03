#include "Slic3r/App/TestRenderModule.hpp"
#include "imgui/imgui.h"
#include <iostream>
#include <chrono>
#include <fmt/chrono.h>

#include <Slic3r/Assert.hpp>

#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/ShaderManager.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"

#include "libslic3r/TriangleMesh.hpp"

namespace Slic3r::App {

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

    // Test asserts (being compilable)
    std::cout << DEBUG_ASSERT_VAL(2, "extra message") << "\n";
    std::cout << DEBUG_ASSERT_VAL(5 != 3) << "\n";

    DEBUG_ASSERT(1);
    DEBUG_ASSERT(1, "extra message");

    std::cout << ASSERT_VAL(2, "extra message") << "\n";
    std::cout << ASSERT_VAL(5 != 3) << "\n";

    ASSERT(1);
    ASSERT(1, "extra message");

    if (false) {
        PANIC();
        PANIC("Extra message");
    }
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
    opts.flip_y = true;
    m_tex = Render::Context::instance().texture_manager().get(
//        "icons/PrusaSlicer-gcodeviewer-mac_128px.png",
//         "icons/funnel.svg",
        "icons/PrusaSlicer-gcodeviewer.svg",
        opts
    );
//    m_tex = Render::Context::instance().texture_manager()
//                .create_empty("white", Render::PixelFormat::RGBA8, 16, 16);

    m_scene = std::make_unique<Scene::Scene>();
    m_scene->camera().set_perspective(90, m_screen_info.logical_width() / m_screen_info.logical_height(), 0.1f, 100);
    Transform3f cam_xform = Transform3f::Identity();
    cam_xform.translate(Vec3f{0, 0 , 10});
    m_scene->camera().model() = cam_xform.matrix();

    auto* n1 = new Scene::Node();
    Transform3f t = Transform3f::Identity();
    t.translate(Vec3f{0, 1, 0});
    n1->set_local_transform(t.matrix());

    auto* n2 = new Scene::Node();
    t = Transform3f::Identity();
    //t.translate(Vec3f{1, 0, 3});
    n2->set_local_transform(t.matrix());
    const indexed_triangle_set& mesh = its_make_cone(2, 5);
    auto* node_geom = m_scene->geometry_manager().get_or_create("cone-2-5", [&]() {
        return Render::geometry_from_triangle_mesh(device, mesh);
    });
    auto* render_component = new Scene::MeshRenderNodeComponent(node_geom);
    n2->set_render_component(
        render_component,
        Scene::Material{device.context().shader_manager().get_shader("gouraud")}
            .set_uniform("uniform_color", ColorRGBA{1.f, 0.6f, 0, 1})
    );
    n1->add_child(n2);
    m_scene->root().add_child(n1);
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

//    SPDLOG_INFO(
//        "Setting viewport to {}x{}", m_screen_info.physical_width(), m_screen_info.physical_height()
//    );

    Transform3f view = Transform3f::Identity();
    view = view.translate(Vec3f(0, 0, -2));
    SPDLOG_TRACE("TestRenderModule::render_scene() 2");
    command_buffer->bind_shader(*m_shader);
    //m_shader->bind();
    SPDLOG_TRACE("TestRenderModule::render_scene() 3");

    command_buffer->bind_geometry(*m_geometry, *m_shader);
    Matrix4f vm = view.matrix();
    Matrix4f proj = Render::perspective(
        60,
        float(m_screen_info.physical_width()) / float(m_screen_info.physical_height()),
        0.01, 10
    );

    command_buffer->set_blending({
        {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha},
        {Render::BlendFactor::One, Render::BlendFactor::OneMinusSrcAlpha}
    });
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
    command_buffer->set_blending_enabled(false);
    command_buffer->submit();

    m_scene->render(*m_device);
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

        double a = 5005.5;
        std::stringstream ss;
        ss << a;
        ImGui::Text("%s", ss.str().c_str());
        char sss[50];
        sprintf(sss, "%g", a);
        ImGui::Text("%s", sss);

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

void TestRenderModule::on_screen_resized()
{
    m_scene->camera().set_perspective(90, m_screen_info.logical_width() / m_screen_info.logical_height(), 0.01, 100);
}

}
