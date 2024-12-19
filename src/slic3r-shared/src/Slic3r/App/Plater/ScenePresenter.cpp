#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::App::Plater {

static const std::unordered_map<ModelVolumeType, ColorRGBA> VOLUME_COLORS = {
    {ModelVolumeType::MODEL_PART, {1, 0.5f, 0, 1}},
    {ModelVolumeType::NEGATIVE_VOLUME, {0.5f, 0.5f, 0.5f, 0.5f}},
    {ModelVolumeType::SUPPORT_BLOCKER, {0.6f, 0.2f, 1.0f, 0.5f}},
    {ModelVolumeType::SUPPORT_ENFORCER, {0.6f, 0.2f, 1.0f, 0.5f}},
    {ModelVolumeType::PARAMETER_MODIFIER, {1, 1.0f, 0, 0.5f}},
    {ModelVolumeType::INVALID, {1, 0.2f, 0.2f, 0.5f}},
};

ScenePresenter::ScenePresenter(
    const Domain::Workbench& m_workbench, Biz::ProjectInteractor& project_interactor, Render::Device& device
)
    : m_workbench(m_workbench), m_project_interactor(project_interactor), m_device(device)
{
//    std::for_each(m_workbench.projects().begin(), m_workbench.projects().end(), [this](const auto& p) {
//        m_projects.emplace(p.first, ScenePresenterProjectContext{});
//    });
    ScenePresenter::on_selected_project_changed(m_project_interactor.selected_project_id());
}

void ScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty())
        project_context().scene().render(command_buffer, this);
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
    if (m_projects.count(m_selected_project_id) == 0) {
        ScenePresenterProjectContext context{};
        m_projects.emplace(m_selected_project_id, std::move(context));
    }
}

Scene::Node* ScenePresenter::initialize_selection_root(Scene::Scene& scene)
{
    Scene::NodeBuilder builder(scene);
    Scene::Node* selection_root = builder
        .set_debug_name("selection_root")
        .set_screen_space_sized_modifier(0.0075)
        .build().release();
    scene.add_child(selection_root);
    return selection_root;
}

void ScenePresenter::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
{
    auto& proj = m_projects[project_id];
    auto& selection_changes = proj.selection_scene_changes();
    auto& scene = proj.scene();

    selection_changes.roll_back();

    bool selection_empty = selection.elements.empty();
    proj.selection_root().set_enabled(!selection_empty);

    if (selection_empty)
        return;

    Scene::Node::NodeList found_nodes;
    found_nodes.reserve(selection.elements.size());
    for (const auto& e : selection.elements) {
        scene.root().query([&](const Scene::Node* n) {
            const auto* tag = n->tag_of_type<SceneNodeTag>();
            if (tag == nullptr)
                return false;
            return tag->matches_element(e);
        }, found_nodes);
    }

    auto selection_mat = Render::Material{}
        .set_uniform("uniform_color", ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f})
        .set_transparent(false);
    for (auto* n : found_nodes)
        selection_changes.change(*n)
            .set_material_override(selection_mat);

    // update selection root, so it is in the center of all selected objects

    Eigen::AlignedBox3f bounds;
    for (const auto& n : found_nodes) {
        // visit all children to find all potential bounding boxes
        // this is important for instance-mode of selection where `n` itself has
        // no bounding box/raycast component
        visit(*n, [&](const Scene::Node& ni) {
            auto* collision = ni.raycast_component();
            if (collision != nullptr) {
                auto wbb = collision->world_bounding_box(n->world_transform());
                for (size_t i = 0; i < 8; i++)
                    bounds.extend(wbb.corner(static_cast<decltype(wbb)::CornerType>(i)));
            }
        });
    }
    proj.set_selection_bounding_box(bounds);
    Matrix4d xform = Matrix4d::Identity();
    //xform.block<1, 3>(0, 3) = bounds.center().cast<double>();
    xform.col(3).head(3) = bounds.center().cast<double>();
    proj.selection_root().set_world_transform(xform);
}

void ScenePresenter::on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
{
    on_scene_selection_changed(project_id, selection);
}

void ScenePresenter::build_volume_node(
    Scene::NodeBuilder& builder,
    Domain::SelectionId project_id,
    const ModelInstance* inst,
    const ModelVolume* vol
)
{
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();

    GeometryElementId id{GeometryElementId::Type::Volume, vol->id().id};
    const auto& trimesh =
        trimesh_mgr.get_or_create(id, [&]() -> std::unique_ptr<Scene::TriangleMesh> {
            return std::make_unique<Scene::TriangleMesh>(vol->mesh_ptr());
        });
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });
    ColorRGBA color = ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};

    auto color_it = VOLUME_COLORS.find(vol->type());
    if (color_it != VOLUME_COLORS.end())
        color = color_it->second;
    const bool transparent = color.a() < 1.0f;

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(transparent);
    builder
        .set_debug_name(fmt::format("vol: {}", vol->id().id))
        .transform([&](auto& xform) { xform = vol->get_matrix(); })
        .set_tag(SceneNodeTag{vol->get_object()->id().id, vol->id().id, inst->id().id, vol->type()})
        .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects))
        .set_aabb(trimesh->aabb_mesh());
}

void ScenePresenter::on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    auto& scn = scene();
    const Domain::Project& project = m_workbench.project(project_id);

    Scene::NodeBuilder builder(scn);
    for (const auto& element : instances) {
        const ModelObject* obj = project.find_object_by_id(element.object_id);
        const ModelInstance* inst = Domain::find_by_id<ModelInstance>(obj->instances, element.instance_id);
        builder
            .set_debug_name(fmt::format("obj: {} inst: {}", obj->id().id, inst->id().id))
            .transform([inst](auto& t) { t = inst->get_matrix(); })
            .set_tag(SceneNodeTag{obj->id().id, 0, inst->id().id, ModelVolumeType::INVALID})
            .child_for_each(obj->volumes, [&](Scene::NodeBuilder& builder, const ModelVolume* vol) {
                build_volume_node(builder, project_id, inst, vol);
            });
        scn.add_child(builder.build().release());
    }
}

void ScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{

}

void ScenePresenter::on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{
    auto& scene = m_projects[m_selected_project_id].scene();
    const auto& proj = m_workbench.project(project_id);
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t == nullptr || t->volume_id != 0)
            return;
        for (const auto& e : elements) {
            if (t->instance_id == e.instance_id) {
                const auto* inst = proj.find_instance_by_id(e.object_id, e.instance_id);
                n.set_local_transform(inst->get_matrix().matrix());
            }
        }
    });
}


void ScenePresenter::on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    // find all instances of given object id and insert the volume node as child
    DEBUG_ASSERT(volumes.size() > 0);
    const auto obj_id = volumes.front().object_id;
    DEBUG_ASSERT(std::all_of(volumes.begin(), volumes.end(), [=](const Domain::ElementRef& vol) {
        return vol.object_id == obj_id;
    }));
    auto& scene = m_projects[project_id].scene();
    const auto* obj = m_workbench.project(project_id).find_object_by_id(obj_id);;

    Scene::visit_conditional(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t != nullptr && t->volume_id == 0 && t->object_id == obj_id) {
            // root of the instance

            const auto* inst = Domain::find_by_id<ModelInstance>(obj->instances, t->instance_id);
            Scene::NodeBuilder builder{scene};
            for (const auto& e : volumes) {
                const auto* vol = Domain::find_by_id<ModelVolume>(obj->volumes, e.volume_id);
                build_volume_node(builder, project_id, inst, vol);
                scene.add_child(builder.build().release(), &n);
            }

            return false;
        }
        return true;
    });
}

void ScenePresenter::on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    // find all instances of given object id and remove the volume node there

}

void ScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{
    auto& scene = m_projects[m_selected_project_id].scene();
    const auto& proj = m_workbench.project(project_id);
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t == nullptr || t->volume_id == 0)
            return;
        for (const auto& e : elements) {
            if (t->volume_id == e.volume_id) {
                const auto* vol = proj.find_volume_by_id(e.object_id, e.volume_id);
                n.set_local_transform(vol->get_matrix().matrix());
            }
        }
    });

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


void ScenePresenter::on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx)
{
    MinimalSceneRenderCustomizer::on_layer_begin(cmd_buf, layer_idx);
    if (layer_idx == int(PlaterSceneLayer::GizmoHandles))

        // clear depth buffer so all gizmo handles are rendered over document objects
        cmd_buf.clear_buffers(false, true);
}


} // namespace Slic3r::App::Plater
