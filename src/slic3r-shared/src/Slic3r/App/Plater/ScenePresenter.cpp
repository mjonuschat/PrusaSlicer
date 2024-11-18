#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::App::Plater {

ScenePresenter::ScenePresenter(
    const Domain::Workbench& m_workbench, Biz::ProjectInteractor& project_interactor, Render::Device& device
)
    : m_workbench(m_workbench), m_project_interactor(project_interactor), m_device(device)
{
//    std::for_each(m_workbench.projects().begin(), m_workbench.projects().end(), [this](const auto& p) {
//        m_projects.emplace(p.first, ScenePresenterProjectContext{});
//    });
    on_selected_project_changed(m_project_interactor.selected_project_id());
}

void ScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty())
        project_context().scene().render(command_buffer);
}

void ScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        project_context().scene().render_imgui(screen_info);
}


void ScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

void ScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0)
        m_projects.emplace(m_selected_project_id, ScenePresenterProjectContext{});
}

void ScenePresenter::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
{

}

void ScenePresenter::on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    auto& scn = scene();
    auto& ctx = project_context();
    auto& geom_mgr = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();
    auto& proj = m_workbench.project(m_selected_project_id);
    const Domain::Project& project = m_workbench.project(project_id);

    Scene::NodeBuilder node_builder(scn);
    node_builder.child_for_each(instances, [&](Scene::NodeBuilder& builder, const Domain::ElementRef& element) {
        const ModelObject* obj = project.find_object_by_id(element.object_id);
        const ModelInstance* inst = Domain::find_by_id<ModelInstance>(obj->instances, element.instance_id);
            builder
                .transform([inst](auto& t) { t = inst->get_matrix(); })
                //.set_tag(SceneNodeTag{obj->id().id, 0, inst->id().id})
                .child_for_each(obj->volumes, [&](Scene::NodeBuilder& builder, const ModelVolume* vol) {
                    GeometryElementId id{GeometryElementId::Type::Volume, element.volume_id};
                    const auto& trimesh =
                        trimesh_mgr.get_or_create(id, [&]() -> std::unique_ptr<Scene::TriangleMesh> {
                            return std::make_unique<Scene::TriangleMesh>(vol->mesh_ptr());
                        });
                    const auto* geom = geom_mgr.get_or_create(id, [&]() {
                        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
                    });
                    auto material = Scene::Material{}
                        .set_shader(m_device.context().shader_manager().get_shader(
                            "gouraud_light"
                        ))
                        .set_uniform("uniform_color", ColorRGBA{1, 0, 0.5f, 1});
                    builder
                        .transform([&](auto& xform) { xform = vol->get_matrix(); })
                        .set_tag(SceneNodeTag{obj->id().id, vol->id().id, inst->id().id})
                        .set_mesh(geom, material)
                        .set_aabb(trimesh->aabb_mesh());
                });
            }
    );
    scn.add_child(node_builder.build().release());
}

void ScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{

}

void ScenePresenter::on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{

}


void ScenePresenter::on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}

void ScenePresenter::on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}

void ScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{

}

void ScenePresenter::on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}


void ScenePresenter::on_bed_added(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_bed_removed(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_bed_transformed(Domain::SelectionId project_id, size_t idx)
{

}


void ScenePresenter::on_wipe_tower_added(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx)
{

}

void ScenePresenter::on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx)
{

}



} // namespace Slic3r::App::Plater
