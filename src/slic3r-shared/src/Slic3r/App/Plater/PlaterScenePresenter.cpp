#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedRenderHelper.hpp"
#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/App/Render/FramebufferManager.hpp"
#include "Slic3r/App/Scene/BedNodeBuilder.hpp"

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

namespace {
template <typename TagT, typename  RefT>
void remove_children(
    Scene::Scene& scn,
    const std::vector<RefT>& elements,
    const std::function<bool(const TagT&, const RefT&)>& predicate
)
{
    // find all instances of given object id and remove the volume node there
    Scene::Node::NodeList nodes;
    scn.root().query([&](const Scene::Node* n) {
        const auto* tag = n->tag_of_type<TagT>();
        if (tag != nullptr) {
            auto it = std::find_if(elements.begin(), elements.end(),
                [tag, predicate](const RefT& el) {
                    return predicate(*tag, el);
                });
            if (it != elements.end())
                return true;
        }
        return false;

    }, nodes, true);
    for (auto* n : nodes)
        scn.remove_child(n);

}

} // namespace (anonymous)



PlaterScenePresenter::PlaterScenePresenter(
    const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor, Render::Device& device
)
    : m_workbench(workbench), m_project_interactor(project_interactor), m_device(device)
    , m_bed_render_updater(*this, workbench, device)
{
    load_selected_project();

    m_project_interactor.add_listener<ISelectedProjectChangedListener>(&m_bed_render_updater);
    m_project_interactor.add_listener<ISelectedProjectChangedListener>(this);

    auto& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.add_listener<ISceneChangedListener>(this);
    scene_interactor.add_listener<ISceneSelectionChangedListener>(this);
    scene_interactor.add_listener<ISelectedBedInstanceChangedListener>(this);
}

void PlaterScenePresenter::load_selected_project()
{
    size_t project_id = m_project_interactor.selected_project_id();
    PlaterScenePresenter::on_selected_project_changed(project_id);
    const auto& p = m_workbench.project(project_id);
    Domain::BedRefs updated_beds;
    Domain::ElementRefs updated_obj_instances;
    for (const auto& cc : p.config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            updated_beds.push_back(Domain::BedRef{cc->id().id, bi->id().id});
            for (const auto& mi : bi->model_instances) {
                updated_obj_instances.emplace_back(mi->get_object()->id().id, mi->id().id, 0);
            }
        }
    }
    for (const auto& mi : p.unplaced_model_instances()) {
        updated_obj_instances.emplace_back(mi->get_object()->id().id, mi->id().id, 0);
    }

    Domain::ElementRefs updated_obj_volumes;
    for (const auto* obj : p.model().objects) {
        ASSERT(!obj->instances.empty());
        auto obj_id = obj->id().id;
        auto inst_id = obj->instances.front()->id().id;
        for (const auto* vol : obj->volumes) {
            updated_obj_volumes.emplace_back(obj_id, inst_id, vol->id().id);
        }
    }

    ASSERT(updated_obj_instances.empty() == updated_obj_volumes.empty());

    PlaterScenePresenter::on_bed_instance_added(project_id, updated_beds);
    if (!updated_obj_instances.empty()) {
        on_instance_added(project_id, updated_obj_instances);
        on_volume_added(project_id, updated_obj_volumes);
    }
}

void PlaterScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty()) {
        Scene::SceneRenderFlag flags = Scene::SceneRenderFlag(
            uint32_t(Scene::SceneRenderFlag::Shadows) |
            uint32_t(Scene::SceneRenderFlag::AmbientOcclusion)
        );
        project_context().scene().render(m_device, command_buffer, this, flags);
    }
}

void PlaterScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        project_context().scene().render_imgui(screen_info);
}

void PlaterScenePresenter::screen_resized(const Render::Rect& viewport)
{
    m_viewport = viewport;
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PlaterScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

void PlaterScenePresenter::update_objects_shadows_data()
{
    const Domain::BedInstance& bed_inst = selected_bed_instance();
    const Domain::ModelInstanceList& insts_on_bed = bed_inst.model_instances;
    const Slic3r::Model* model = &m_project_interactor.selected_project().model();

    auto& scene = m_projects[m_project_interactor.selected_project_id()].scene();
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* tag = n.tag_of_type<SceneNodeTag>();
        if (tag != nullptr && n.has_render_component()) {
            const auto* obj = Domain::find_by_id<ModelObject>(model->objects, tag->object_id);
            const auto* vol = Domain::find_by_id<ModelVolume>(obj->volumes, tag->volume_id);
            if (vol->is_model_part()) {
                const auto* inst = Domain::find_by_id<ModelInstance>(obj->instances, tag->instance_id);
                bool shadows = std::find(insts_on_bed.begin(), insts_on_bed.end(), inst) != insts_on_bed.end();
                n.render_component()->set_shadows(shadows ? Render::Shadows{ true, true } : Render::Shadows{ false, false });
            }
            else
                n.render_component()->set_shadows(Render::Shadows{ false, false });
        }
    }, true);
}

void PlaterScenePresenter::update_beds_shadows_data()
{
    m_bed_render_updater.update_shadows(project_context().scene().camera());
}

void PlaterScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        m_projects.try_emplace(m_selected_project_id);
        m_bed_render_updater.on_selected_project_changed(m_selected_project_id);
        // a new camera has been created, add the bed updater as listener
        auto& camera = project_context().scene().camera();
        camera.add_listener<Scene::ICameraUpdateListener>(&m_bed_render_updater);
        camera.set_viewport(m_viewport);
    }
}

void PlaterScenePresenter::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
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
        .set_uniform("uniform_color", ColorRGBA::WHITE())
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
                auto wbb = collision->world_bounding_box(ni.world_transform());
                for (size_t i = 0; i < 8; i++)
                    bounds.extend(wbb.corner(static_cast<decltype(wbb)::CornerType>(i)));
            }
        });
    }
    proj.set_selection_bounding_box(bounds);
    if (!m_freeze_selection_center) {
        Matrix4d xform = Matrix4d::Identity();
        if (selection.mode == Biz::Scene::SelectionMode::Instance) {
            const auto* tag = found_nodes.front()->tag_of_type<SceneNodeTag>();
            const ModelObject* obj = m_project_interactor.selected_project().find_object_by_id(tag->object_id);
            const ModelInstance* inst = m_project_interactor.selected_project().find_instance_by_id(tag->object_id, tag->instance_id);
            Geometry::Transformation world_m = inst->get_transformation() * obj->volumes.front()->get_transformation();
            xform.col(3).head(3) = world_m.get_offset();
        }
        else {
            //xform.block<1, 3>(0, 3) = bounds.center().cast<double>();
            xform.col(3).head(3) = bounds.center().cast<double>();
        }
        proj.selection_root().set_world_transform(xform);
    }
}

void PlaterScenePresenter::on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::Selection& selection)
{
    on_scene_selection_changed(project_id, selection);
}

void PlaterScenePresenter::on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id)
{
    m_bed_render_updater.update_all(project_context().scene().camera());
    const Domain::BedInstance& bed_inst = selected_bed_instance();
    Domain::Vec3d bed_inst_offset = bed_inst.transformation.get_offset();
    const Domain::Bed& bed = bed_inst.bed;
    std::vector<Domain::Vec3f> print_volume = Biz::Scene::BedGeometry::print_volume(bed);
    Eigen::AlignedBox3d bed_aabb;
    for (const auto& v : print_volume) {
        bed_aabb.extend(bed_inst_offset + v.cast<double>());
    }
    scene().set_bed_aabb(bed_aabb);
    update_objects_shadows_data();
}

void PlaterScenePresenter::build_volume_node(
    Scene::NodeBuilder& builder,
    Domain::SelectionId project_id,
    const ModelInstance* inst,
    const ModelVolume* vol
)
{
    SPDLOG_DEBUG("build_volume inst: {}  vol: {}", inst->id().id, vol->id().id);
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();

    Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::Volume, vol->id().id};
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

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    builder
        .set_debug_name(fmt::format("vol: {}", vol->id().id))
        .transform([vol](auto& xform) { xform = vol->get_matrix(); })
        .set_tag(SceneNodeTag{vol->get_object()->id().id, vol->id().id, inst->id().id, vol->type()})
        .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects))
        .set_aabb(trimesh->aabb_mesh());
    if (vol->type() == ModelVolumeType::MODEL_PART) {
        builder
            .set_shadows(Render::Shadows{ true, true })
            // FIXME: the pbr data should be set in dependence of the volume filament 
            // see PrusaSlicer PrintConfigDef::init_fff_params() option 'filament_type' 
            .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS);
    }
    else {
        builder
            .set_shadows(Render::Shadows{ false, false });
    }
}

const Domain::BedInstance& PlaterScenePresenter::selected_bed_instance() const
{
    return m_project_interactor.selected_config_container()
        .find_bed_instance(m_project_interactor.scene_interactor().selected_bed_instance().instance_id);
}

void PlaterScenePresenter::on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
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
            // .child_for_each(obj->volumes, [&](Scene::NodeBuilder& builder, const ModelVolume* vol) {
            //     build_volume_node(builder, project_id, inst, vol);
            // })
            ;
        scn.add_child(builder.build().release());
    }
}

void PlaterScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        instances,
        [](const auto& tag, const auto& el) {
            return tag.object_id == el.object_id && tag.instance_id == el.instance_id;
        }
    );
}

void PlaterScenePresenter::on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{
    const Domain::BedInstance& bed_inst = selected_bed_instance();
    const Domain::ModelInstanceList& insts_on_bed = bed_inst.model_instances;

    auto& scene = m_projects[m_selected_project_id].scene();
    const auto& proj = m_workbench.project(project_id);
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t == nullptr)
            return;
        if (t->volume_id == 0) {
            for (const auto& e : elements) {
                if (t->instance_id == e.instance_id) {
                    const auto* inst = proj.find_instance_by_id(e.object_id, e.instance_id);
                    n.set_local_transform(inst->get_matrix().matrix());
                }
            }
        }
        else {
            if (std::find_if(elements.begin(), elements.end(),
                [t](auto e) { return t->instance_id == e.instance_id; }) != elements.end()) {
                const auto* vol = proj.find_volume_by_id(t->object_id, t->volume_id);
                if (vol->is_model_part()) {
                    bool shadows = std::find_if(insts_on_bed.begin(), insts_on_bed.end(),
                        [t](auto i) { return i->id().id == t->instance_id; }) != insts_on_bed.end();
                        n.render_component()->set_shadows(shadows ? Render::Shadows{ true, true } : Render::Shadows{ false, false });
                }
                else
                    n.render_component()->set_shadows(Render::Shadows{ false, false });
            }
        }
    });
}


void PlaterScenePresenter::on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    // find all instances of given object id and insert the volume node as child
    DEBUG_ASSERT(volumes.size() > 0);

    std::set<size_t> object_ids;
    for (const auto& v : volumes)
        object_ids.insert(v.object_id);
    // const auto obj_id = volumes.front().object_id;
    // DEBUG_ASSERT(std::all_of(volumes.begin(), volumes.end(), [=](const Domain::ElementRef& vol) {
    //     return vol.object_id == obj_id;
    // }));
    auto& scene = m_projects[project_id].scene();
    // const auto* obj = m_workbench.project(project_id).find_object_by_id(obj_id);

    Scene::visit_conditional(scene.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t != nullptr && t->volume_id == 0 && object_ids.contains(t->object_id)) {
            // root of the instance
            const auto* obj = m_workbench.project(project_id).find_object_by_id(t->object_id);
            const auto* inst = Domain::find_by_id<ModelInstance>(obj->instances, t->instance_id);
            Scene::NodeBuilder builder{scene};
            for (const auto& e : volumes) {
                if (e.object_id != t->object_id)
                    continue;
                const auto* vol = Domain::find_by_id<ModelVolume>(obj->volumes, e.volume_id);
                build_volume_node(builder, project_id, inst, vol);
                scene.add_child(builder.build().release(), &n);
            }

            return false;
        }
        return true;
    });
}

void PlaterScenePresenter::on_volume_removed(
    Domain::SelectionId project_id, const Domain::ElementRefs& volumes
)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        volumes,
        [](const SceneNodeTag& tag, const Domain::ElementRef& el) {
            return tag.object_id == el.object_id && tag.volume_id == el.volume_id;
        }
    );
}

void PlaterScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements)
{
    const Domain::BedInstance& bed_inst = selected_bed_instance();
    const Slic3r::Model* model = &m_project_interactor.selected_project().model();

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
                if (vol->is_model_part()) {
                    Scene::visit(scene.root(), [&](Scene::Node& n) {
                        const SceneNodeTag* tag = n.tag_of_type<SceneNodeTag>();
                        if (tag != nullptr && n.has_render_component()) {
                            if (tag->object_id == t->object_id && tag->volume_id == t->volume_id) {
                                const auto* obj = Domain::find_by_id<ModelObject>(model->objects, tag->object_id);
                                const auto* inst = Domain::find_by_id<ModelInstance>(obj->instances, tag->instance_id);
                                bool shadows = bed_inst.contains(Biz::Algorithms::BoundingBox::to_2d(transformed(vol->mesh().bounding_box(), inst->get_matrix() * vol->get_matrix())));
                                n.render_component()->set_shadows(shadows ? Render::Shadows{ true, true } : Render::Shadows{ false, false });
                            }
                        }
                    });
                }
                else
                    n.render_component()->set_shadows(Render::Shadows{ false, false });
            }
        }
    });

}

void PlaterScenePresenter::on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{

}

void PlaterScenePresenter::on_bed_instance_added(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    auto& scn = scene();
    const auto& proj = m_workbench.project(project_id);

    for (auto& instance : instances) {
        const Domain::ConfigContainer* cc = proj.find_config_container(instance.config_container_id);
        DEBUG_ASSERT(cc != nullptr);
        const Domain::BedInstance& inst = cc->find_bed_instance(instance.instance_id);

        Scene::BedNodeTag tag = {instance.config_container_id, instance.instance_id};

        Scene::NodeBuilder builder(scn);
        Scene::BedNodeBuilder::bed_node(builder, inst, tag, m_device, m_projects[project_id],
            int(PlaterSceneLayer::DocumentObjects));

        scn.add_child(builder.build().release());
    }

    m_bed_render_updater.update_all(scn.camera());
}

void PlaterScenePresenter::on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    remove_children<Scene::BedNodeTag, Domain::BedRef>(
        scene(),
        instances,
        [](const Scene::BedNodeTag& tag, const Domain::BedRef& br) {
            return tag.config_container_id == br.config_container_id && tag.instance_id == br.instance_id;
        }
    );

    m_bed_render_updater.update_all(scene().camera());
}

void PlaterScenePresenter::on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
}

void PlaterScenePresenter::on_wipe_tower_added(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id)
{

}

void PlaterScenePresenter::on_wipe_tower_removed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id)
{

}

void PlaterScenePresenter::on_wipe_tower_transformed(Domain::SelectionId project_id, Domain::SelectionId  wipe_tower_id)
{

}

void PlaterScenePresenter::on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx)
{
    cmd_buf.set_depth_write_enabled(true);
    if (layer_idx == int(PlaterSceneLayer::GizmoHandles))

        // clear depth buffer so all gizmo handles are rendered over document objects
        cmd_buf.clear_buffers(false, true);
}


} // namespace Slic3r::App::Plater
