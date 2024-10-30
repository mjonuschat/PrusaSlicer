#include "Slic3r/App/Plater/PlaterRenderModule.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Plater/CameraGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Plater/QuickDragGizmo.hpp"

#include "imgui/imgui.h"

namespace Slic3r::App::Plater {

void PlaterRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    init_scene();
    init_gizmos();
}

void PlaterRenderModule::init_scene()
{
    auto& device  = *m_device;
    m_scene = std::make_unique<Scene::Scene>();
    m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    Transform3f cam_xform = Transform3f::Identity();
    cam_xform.translate(Vec3f{0, 0 , 30});
    m_scene->camera().model() = cam_xform.matrix();

    Scene::NodeBuilder node_builder{*m_scene};
    auto* cone_mesh = m_scene->triangle_mesh_manager().get_or_create("cone-2-5", [](){
        return std::make_unique<Scene::TriangleMesh>(its_make_cone(2, 5, M_PI / 8));
    });
    auto* cube_mesh = m_scene->triangle_mesh_manager().get_or_create("box-2-2-2", [](){
        return std::make_unique<Scene::TriangleMesh>(its_make_cube(2, 2, 2));
    });

    auto cone = m_scene->geometry_manager().get_or_create("cone-2-5", [&]() {
        return Render::geometry_from_triangle_mesh(device, cone_mesh->triangles);
    });

    auto cube = m_scene->geometry_manager().get_or_create("box-2-2-2", [&](){
        return Render::geometry_from_triangle_mesh(device, cube_mesh->triangles);
    });

    auto shader = device.context().shader_manager().get_shader("gouraud_light");
    constexpr float right_angle = M_PI / 2.0f;

    node_builder
        //        .transform([](auto& xform){
        //            xform
        //                .rotate(Eigen::AngleAxisf{right_angle/2, Vec3f::UnitZ()})
        //                .rotate(Eigen::AngleAxisf{right_angle/2, Vec3f::UnitX()});
        //        })
        .child([&](auto& node_builder){
            node_builder
                .set_screen_space_sized_modifier(0.03f)
                .children(3, [&](auto& builder, size_t i){
                    ColorRGBA color{0, 0, 0, 1.f};
                    color.set(i, 1);
                    builder
                        .transform([&](auto& xform) {
                            Vec3f offset = Vec3f::Zero();
                            offset[i] = 10;
                            xform.translate(offset);
                        })
                        .child([&](auto& builder){
                            builder
                                .transform([&](auto& xform){
                                    // cone points in +Z direction,
                                    if (i == 0) {
                                        // make it pointing in +X direction
                                        xform.rotate(Eigen::AngleAxisf{right_angle, Vec3f::UnitY()});
                                    } else if (i == 1) {
                                        // make it pointing in +Y direction
                                        xform.rotate(Eigen::AngleAxisf{-right_angle, Vec3f::UnitX()});
                                    }
                                })
                                .set_mesh(
                                    cone,
                                    Scene::Material{}
                                        .set_shader(shader)
                                        .set_uniform("uniform_color", color),
                                    100
                                )
                                .set_aabb(cone_mesh->aabb_mesh.get())
                                .set_tag(GizmoNodeTag{.primary_axis=AxisType(i)});

                            if (i == 0)
                                builder.set_imgui_func([this](const auto& n, const auto& screen_bb) {
                                    this->render_object_hud(n, screen_bb);
                                });

                        });
                });
        })
        .children(5 * 5, [&](auto& builder, size_t idx) {
            builder
                .transform([idx](auto& xform){
                    constexpr static float stride = 10;
                    xform.translate(Vec3f{(idx % 5) * stride, (idx / 5) * stride, 0});
                })
                .child([&](auto& builder){
                    builder
                        .transform([](auto& xform) {
                            xform.translate(Vec3f{-1, -1, -1});
                        })
                        .set_mesh(
                            cube,
                            Scene::Material{}
                                .set_shader(shader)
                                .set_uniform("uniform_color", ColorRGBA{1.0f, 0.0f, 0.5f, 1.0f})
                        )
                        .set_aabb(cube_mesh->aabb_mesh.get())
                        .set_tag(SceneNodeTag{.object_id=0, .volume_id=0, .instance_id=idx});
                });
        });
    m_scene->add_child(node_builder.build().release());
    m_scene->camera_trackball().set_focal_distance(30);
}

void PlaterRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<GizmoManager>(*m_scene);
    m_gizmo_manager->add_base_gizmo<CameraGizmo>(*m_scene);
    m_gizmo_manager->add_base_gizmo<QuickSelectGizmo>(*m_scene);
    m_gizmo_manager->add_base_gizmo<QuickDragGizmo>();
}


void PlaterRenderModule::render_scene()
{
    m_device->load_state();
    auto cmd_buffer = m_device->create_command_buffer();

    cmd_buffer->set_clear_values({0.45f, 0.55f, 0.60f, 1.00f});
    cmd_buffer->clear_buffers(true, true);
    cmd_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));

    m_scene->render(*cmd_buffer);
    cmd_buffer->submit();
}

void imgui_scenegraph_node_info(const Scene::Node& node)
{
    ImGuiTreeNodeFlags node_flags = 0;
    if (node.children().empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    if (ImGui::TreeNodeEx(
            &node, node_flags, "Node %s%s%s%s", node.has_render_component() ? "(R)" : "",
            node.has_material_override() ? "(M)" : "",
            node.has_imgui_render_component() ? "(I)" : "", node.has_raycast_component() ? "(C)" : ""
        )) {
        for (const auto& ch : node.children()) {
            imgui_scenegraph_node_info(*ch);
        }
        ImGui::TreePop();
    }
}

void PlaterRenderModule::render_imgui()
{
    m_scene->render_imgui(m_screen_info);

    if (ImGui::Begin("Slicer", &m_gui_win_open)) {
        imgui_scenegraph_node_info(m_scene->root());
    }
    ImGui::End();
}

void PlaterRenderModule::render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box)
{
    std::string node_name = "##node_hud_" + std::to_string(reinterpret_cast<long>(&n));

    ImGui::SetNextWindowPos({
        screen_bounding_box.max().x(),
        screen_bounding_box.min().y()
    });
    if (ImGui::Begin(node_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground)) {
        if (ImGui::SmallButton("Foc"))
            m_scene->camera_trackball().set_focal_point({0, 0, 0});
    }
    ImGui::End();
}


void PlaterRenderModule::on_scene_mouse_event(
    const Platform::MouseEvent& e
)
{
    m_gizmo_manager->on_scene_mouse_event(e, m_screen_info);
}
void PlaterRenderModule::on_scene_keyboard_event(
    const Platform::KeyboardEvent& e
)
{
    AbstractRenderModule::on_scene_keyboard_event(e);
}

void PlaterRenderModule::on_activated()
{

}
void PlaterRenderModule::on_deactivated()
{

}
void PlaterRenderModule::on_screen_resized()
{
    m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
}


} // namespace Slic3r::App::Plater
