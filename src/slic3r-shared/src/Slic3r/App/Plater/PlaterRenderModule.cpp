#include "Slic3r/App/Plater/PlaterRenderModule.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Plater/CameraGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include "imgui/imgui.h"
#include "Eigen/SVD"

namespace Slic3r::App::Plater {

void PlaterRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    m_scene_presenter =
        std::make_unique<ScenePresenter>(m_workbench, m_project_interactor, *m_device);
    m_project_interactor.add_selected_project_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_selection_changed_listener(m_scene_presenter.get());
    init_gizmos();
    init_scene();
}

void PlaterRenderModule::init_scene()
{
    auto& scene_interactor = m_project_interactor.scene_interactor();

    const size_t x_size = 5;
    const size_t y_size = 5;
    const double span = 20;
    const double x_off = -((x_size - 1) * span) / 2;
    const double y_off = -((y_size - 1) * span) / 2;

    const Domain::Bed* bed = scene_interactor.bed();
    Vec2d bed_center = bed->center().cast<double>();
    Transform3d bed_xform = Geometry::translation_transform(to_3d(-bed_center, 0));
    scene_interactor.new_bed(bed->id().id, bed_xform.matrix());

    // add a second instance of the bed, as inactive
    bed_xform.translate(Vec3d(300, 0, 0));
    scene_interactor.add_bed_instance(bed->id().id, bed_xform.matrix());
    Domain::BedInstance* bed_instance = bed->instances().back();
    bed_instance->set_active(false);
    m_scene_presenter->update_beds();

    {
        scene_interactor.new_object_from_mesh(TriangleMesh{its_make_cube(10,10,10) });

        Biz::Scene::TransformMemento xform_memento;
        Transform3d xform = Transform3d::Identity();
        xform.translate(Vec3d{0 * span + x_off, 0 * span + y_off, 0});
        scene_interactor.transform_selection(xform.matrix(), xform_memento);

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 10, 10, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_cube(10,10,10)}, ModelVolumeType::NEGATIVE_VOLUME, xform.matrix());

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 0, -10, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_cube(10,10,10)}, ModelVolumeType::PARAMETER_MODIFIER, xform.matrix());

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 0, 5, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_sphere(10, 12)}, ModelVolumeType::SUPPORT_ENFORCER, xform.matrix());

    }


    for (size_t x = 0; x < x_size; x++) {
        for (size_t y = 0; y < y_size; y++) {
            if (x == 0 && y == 0)
                continue;

            Transform3d xform = Transform3d::Identity();
            xform.translate(Vec3d{x * span + x_off, y * span + y_off, 0});
            scene_interactor.add_instance(xform.matrix());
        }
    }

    m_scene_presenter->scene().log_nodes();
}

void PlaterRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<GizmoManager>(*m_device, *m_scene_presenter);
    m_gizmo_manager->add_base_gizmo<CameraGizmo>(*m_scene_presenter);
    m_gizmo_manager->add_base_gizmo<QuickSelectGizmo>(m_project_interactor.scene_interactor(), *m_device, *m_scene_presenter, m_screen_info);
    m_gizmo_manager->add_base_gizmo<QuickDragGizmo>(m_project_interactor.scene_interactor(), *m_scene_presenter);
    m_gizmo_manager->add_tool_gizmo<TranslationGizmo>(
            m_gizmo_manager->data_factory(), *m_scene_presenter, m_project_interactor.scene_interactor()
    );
}


void PlaterRenderModule::render_scene()
{
    m_device->load_state();
    auto cmd_buffer = m_device->create_command_buffer();

    cmd_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer->set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer->clear_buffers(true, true);

    m_scene_presenter->render_scene(*cmd_buffer);

    m_gizmo_manager->render_scene(*cmd_buffer);

    cmd_buffer->submit();
}

class ImguiVecRender
{
public:
    void operator()(const char* label, const Vec2f& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }

    void operator()(const char* label, const Vec2d& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }
    //
    // void operator()(const char* label, const Vec3f& v)
    // {
    //     fill_data<3>(v);
    //     ImGui::InputFloat3(label, m_data);
    // }
    //
    void operator()(const char* label, const Vec3d& v)
    {
        fill_data<3>(v);
        ImGui::InputFloat3(label, m_data);
    }

    void operator()(const char* label, const Vec4f& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

    void operator()(const char* label, const Vec4d& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }
private:
    template <size_t N, typename VecT>
    void fill_data(const VecT &data)
    {
        for (size_t i = 0; i < N; i++) m_data[i] = static_cast<float>(data[i]);
    }
private:
    float m_data[4];
};

void imgui_scenegraph_node_info(const Scene::Node& node)
{
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children().empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    const std::string& name = node.debug_name();
    if (ImGui::TreeNodeEx(
            &node, node_flags, "%s %s%s%s%s", name.empty() ? "Node" : name.c_str(),
            node.has_render_component() ? "(R)" : "", node.has_material_override() ? "(M)" : "",
            node.has_imgui_render_component() ? "(I)" : "", node.has_raycast_component() ? "(C)" : ""
        )) {

        if (ImGui::CollapsingHeader("I")) {
            auto transform{node.world_transform()};

            ImguiVecRender vec_render;
            for (size_t i = 0; i < 4; i++)
                vec_render("", Vec4d{transform.row(i)});
        }

        for (const auto& ch : node.children()) {
            imgui_scenegraph_node_info(*ch);
        }
        ImGui::TreePop();
    }
}

void PlaterRenderModule::render_imgui()
{
    if (!m_scene_presenter->project_ready())
        return;

    trl.render(ImVec2(m_screen_info.logical_width(), m_screen_info.logical_height()));

    m_scene_presenter->render_imgui(m_screen_info);

    m_gizmo_manager->render_imgui();

    if (ImGui::Begin("Outline", &m_gui_win_open)) {
        if (ImGui::Button("Translate"))
            // TODO: get and pass the correct printer type
            m_gizmo_manager->toggle_activate_tool(ToolType::Translation, ptFFF);
        imgui_scenegraph_node_info(m_scene_presenter->scene().root());
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
            m_scene_presenter->scene().camera_trackball().set_focal_point({0, 0, 0});
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
    m_gizmo_manager->on_scene_keyboard_event(e);
}

void PlaterRenderModule::on_activated()
{

}
void PlaterRenderModule::on_deactivated()
{

}
void PlaterRenderModule::on_screen_resized()
{
    //m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    auto viewport = Render::Rect::from(0, 0, m_screen_info);
    m_scene_presenter->update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}


} // namespace Slic3r::App::Plater
