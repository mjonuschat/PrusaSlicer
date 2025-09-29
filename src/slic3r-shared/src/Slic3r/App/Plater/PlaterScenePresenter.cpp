#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"

#include <ranges>
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Transformation.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedRenderHelper.hpp"
#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/App/Render/FramebufferManager.hpp"
#include "Slic3r/App/Scene/BedNodeBuilder.hpp"
#include "Slic3r/App/Plater/ThumbnailRenderer.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Biz/Scene/BedTracking.hpp"
#include "Slic3r/App/Scene/PrintVolumeData.hpp"

using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Vec3d;

using Slic3r::Biz::Algorithms::BoundingBox::transformed;
using Slic3r::Biz::Algorithms::Color::saturate;

namespace Slic3r::App::Plater {

static const std::unordered_map<Domain::ModelVolumeType, ColorRGBA> VOLUME_COLORS = {
    {Domain::ModelVolumeType::MODEL_PART,         {1.0f, 0.5f, 0.0f, 1.0f}},
    {Domain::ModelVolumeType::NEGATIVE_VOLUME,    {0.5f, 0.5f, 0.5f, 0.5f}},
    {Domain::ModelVolumeType::SUPPORT_BLOCKER,    {0.6f, 0.2f, 1.0f, 0.5f}},
    {Domain::ModelVolumeType::SUPPORT_ENFORCER,   {0.6f, 0.2f, 1.0f, 0.5f}},
    {Domain::ModelVolumeType::PARAMETER_MODIFIER, {1.0f, 1.0f, 0.0f, 0.5f}},
    {Domain::ModelVolumeType::INVALID,            {1.0f, 0.2f, 0.2f, 0.5f}},
};

namespace {
template <typename TagT, typename RefT>
void remove_children(Scene::Scene& scn, const std::vector<RefT>& elements, const std::function<bool(const TagT&, const RefT&)>& predicate)
{
    // find all instances of given object id and remove the volume node there
    Scene::Node::NodeList nodes;
    scn.root().query(
        [&](const Scene::Node* n)
        {
            const auto* tag = n->tag_of_type<TagT>();
            if (tag != nullptr) {
                auto it = std::find_if(
                    elements.begin(),
                    elements.end(),
                    [tag, predicate](const RefT& el) { return predicate(*tag, el); }
                );
                if (it != elements.end())
                    return true;
            }
            return false;
        },
        nodes,
        true
    );

    // go through the nodes in reverse order, so the children is removed before parent
    for (auto& node : std::ranges::reverse_view(nodes))
        scn.remove_child(node);
}

} // namespace

PlaterScenePresenter::PlaterScenePresenter(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor, Render::Device& device) :
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_device(device),
    m_bed_render_updater(*this, workbench, device, project_interactor.scene_interactor())
{
    load_selected_project();

    m_project_interactor.add_listener<ISelectedProjectChangedListener>(&m_bed_render_updater);
    m_project_interactor.add_listener<ISelectedProjectChangedListener>(this);

    auto& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.add_listener<ISceneChangedListener>(this);
    scene_interactor.add_listener<ISceneBedInstanceChangedListener>(this);
    scene_interactor.add_listener<ISceneSelectionChangedListener>(this);
    scene_interactor.add_listener<ISelectedBedInstancesChangedListener>(this);
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

    PlaterScenePresenter::on_bed_instance_updated(project_id, updated_beds);
    if (!updated_obj_instances.empty()) {
        on_instance_added(project_id, updated_obj_instances);
    }
}

void PlaterScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty()){
        if (m_volume_materials_dirty) {
            update_volume_materials();
            m_volume_materials_dirty = false;
        }

#if ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_scene_aabb_node(project_context(), m_device);
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_camera_frustum(scene().camera());
        scene().render(m_device, command_buffer, this);
    }
}

void PlaterScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        scene().render_imgui(screen_info);
}

void PlaterScenePresenter::screen_resized(const Render::Rect& viewport)
{
    m_viewport = viewport;
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PlaterScenePresenter::force_bed_thumbnails_generation()
{
    invoke_bed_visually_changed(m_selected_project_id);
}

void PlaterScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(
        m_projects.begin(),
        m_projects.end(),
        [modifier](auto& p) { modifier(p.second.scene().camera()); }
    );
}

namespace {
std::pair<Domain::ModelInstanceList, Domain::ModelInstanceList> get_instances_on_beds(const PlaterScenePresenter::BedInstances& bed_instances)
{
    std::pair<Domain::ModelInstanceList, Domain::ModelInstanceList> result;
    for (const auto& bed_instance : bed_instances) {
        const Domain::ModelInstanceList& instances_on_bed{bed_instance.get().model_instances};
        result.first.insert(result.first.end(), instances_on_bed.begin(), instances_on_bed.end());
        const Domain::ModelInstanceList& instances_colliding{ bed_instance.get().colliding_instances };
        result.second.insert(result.second.end(), instances_colliding.begin(), instances_colliding.end());
    }
    return result;
}
} // namespace

static std::unordered_map<Domain::SelectionId, const Domain::BedInstance*> model_instance_to_bed_instance_lookup_map(const Domain::Project& project)
{
    std::unordered_map<size_t, const Domain::BedInstance*> ret;
    for (const auto& cc : project.config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            for (const auto& mi : bi->model_instances) {
                ret[mi->id().id] = bi.get();
            }
        }
    }
    return ret;
}

static std::unordered_map<Domain::SelectionId, const Domain::BedInstance*> collision_instance_to_bed_instance_lookup_map(const Domain::Project& project)
{
    std::unordered_map<Domain::SelectionId, const Domain::BedInstance*> ret;
    for (const auto& cc : project.config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            for (const auto& mi : bi->colliding_instances) {
                ret[mi->id().id] = bi.get();
            }
        }
    }
    return ret;
}

static const Domain::BedInstance* find_bed_instance_by_model_instance_id(const std::unordered_map<size_t, const Domain::BedInstance*>& lookup_map,
    Domain::SelectionId inst_id)
{
    auto it = lookup_map.find(inst_id);
    return (it != lookup_map.end()) ? it->second : nullptr;
}

static ColorRGBA select_color(bool is_model_part, bool is_selected, bool is_outside)
{
    static const ColorRGBA OUTSIDE_COLOR = ColorRGBA(0.0f, 0.38f, 0.8f, 1.0f);
    static const ColorRGBA OUTSIDE_SELECTED_COLOR = ColorRGBA(0.19f, 0.58f, 1.0f, 1.0f);
    static const ColorRGBA SELECTED_COLOR = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);

    ColorRGBA ret = (is_outside && is_selected) ? OUTSIDE_SELECTED_COLOR :
                    is_outside ? OUTSIDE_COLOR :
                    is_selected ?  SELECTED_COLOR : ColorRGBA::BLACK();

    if (!is_model_part)
        ret.a(0.65f);

    return ret;
}

void PlaterScenePresenter::update_volume_materials()
{
    BedInstances bed_instances;
    for (auto& cc : m_project_interactor.scene_interactor().selected_project_config_containers()) {
        for (auto& bed_inst : cc->bed_instances()) {
            if (!bed_inst->model_instances.empty() || !bed_inst->colliding_instances.empty())
                bed_instances.push_back(*bed_inst);
        }
    }

    std::pair<Domain::ModelInstanceList, Domain::ModelInstanceList> instances = get_instances_on_beds(bed_instances);

    BedInstances sel_bed_instances = selected_bed_instances();
    std::pair<Domain::ModelInstanceList, Domain::ModelInstanceList> sel_instances = get_instances_on_beds(sel_bed_instances);

    const Domain::Project& proj = m_project_interactor.selected_project();
    const Biz::Scene::ObjectSelection& selection = m_project_interactor.scene_interactor().object_selection();

    std::unordered_map<Domain::SelectionId, const Domain::BedInstance*> mi_to_bi_map = model_instance_to_bed_instance_lookup_map(proj);
    std::unordered_map<Domain::SelectionId, const Domain::BedInstance*> ci_to_bi_map = collision_instance_to_bed_instance_lookup_map(proj);

    Scene::visit(
        scene().root(),
        [&](Scene::Node& n)
        {
            const SceneNodeTag* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr && n.has_render_component()) {
                const auto* obj = proj.find_object_by_id(tag->object_id);
                const auto* inst = proj.find_instance_by_id(tag->object_id, tag->instance_id);
                bool is_on_bed = std::find(instances.first.begin(), instances.first.end(), inst) != instances.first.end() ||
                    std::find(instances.second.begin(), instances.second.end(), inst) != instances.second.end();
                bool is_on_selected_bed = std::find(sel_instances.first.begin(), sel_instances.first.end(), inst) != sel_instances.first.end();
                bool is_model_part = tag->volume_type == Domain::ModelVolumeType::MODEL_PART;
                n.render_component()->set_shadows((is_model_part && is_on_selected_bed) ?
                    Render::Shadows{true, true} : Render::Shadows{false, false});

                const Domain::BedInstance* bed_inst = nullptr;
                const Domain::Bed* bed = nullptr;
                bool is_colliding = false;
                bool is_selected = selection.is_selected({ obj->id().id, inst->id().id, tag->volume_id });

                ColorRGBA color;
                if (is_on_bed) {
                    bed_inst = find_bed_instance_by_model_instance_id(mi_to_bi_map, inst->id().id);
                    if (bed_inst == nullptr) {
                        bed_inst = find_bed_instance_by_model_instance_id(ci_to_bi_map, inst->id().id);
                        is_colliding = true;
                    }
                    bed = &bed_inst->bed.get();

                    color = select_color(is_model_part, is_selected, is_colliding);
                }
                else
                    color = select_color(is_model_part, is_selected, true);

                if (is_on_bed && !is_colliding && !is_selected)
                    n.remove_material_override();
                else {
                    Render::Material mat = Render::Material{}.set_uniform("uniform_color", color).set_transparent(color.is_transparent());
                    n.set_material_override(mat);
                }

                Scene::PrintVolumeData print_volume;
                if (bed != nullptr) {
                    Domain::Vec2d offset = Biz::Algorithms::Point::to_2d(bed_inst->transformation.get_offset());
                    print_volume.type = bed->type();
                    print_volume.z_data = is_model_part ? Domain::Vec2f(float(Scene::BED_OFFSET_Z), bed->max_print_height()) : Domain::Vec2f(-FLT_MAX, FLT_MAX);
                    if (print_volume.type == Domain::BedType::Circle) {
                        std::optional<Domain::Bed::Circle> circle = bed->circle();
                        print_volume.xy_data = Domain::Vec4f(
                            float(offset.x() + circle->first.x()),
                            float(offset.y() + circle->first.y()),
                            is_model_part ? float(circle->second) : FLT_MAX,
                            FLT_MAX
                        );
                    }
                    else {
                        Domain::Vec2d center = offset + bed->center();
                        Domain::Vec2d half_bbox_size = 0.5 * bed->contour_aabb_extent();
                        print_volume.xy_data = Domain::Vec4f(
                            is_model_part ? float(center.x() - half_bbox_size.x()) : -FLT_MAX,
                            is_model_part ? float(center.y() - half_bbox_size.y()) : -FLT_MAX,
                            is_model_part ? float(center.x() + half_bbox_size.x()) : FLT_MAX,
                            is_model_part ? float(center.y() + half_bbox_size.y()) : FLT_MAX
                        );
                    }
                }
                else {
                    print_volume.type = Domain::BedType::Invalid;
                    print_volume.z_data = Domain::Vec2f(-FLT_MAX, FLT_MAX);
                    print_volume.xy_data = Domain::Vec4f(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
                }

                Render::Material mat = n.render_component()->material();
                set_uniforms(print_volume, mat);
                n.render_component()->replace_material(mat);
                if (n.has_material_override()) {
                    Render::Material ov_mat = *n.material_override();
                    set_uniforms(print_volume, ov_mat);
                    n.set_material_override(ov_mat);
                }
            }
        },
        true
    );
}

void PlaterScenePresenter::update_beds_shadows_data()
{
    m_bed_render_updater.update_shadows(project_context().scene().camera());
}

void PlaterScenePresenter::on_project_loaded(Domain::SelectionId project_id)
{
    center_camera_on_selected_bed();
}

void PlaterScenePresenter::center_camera_on_selected_bed()
{
    // Center the camera on the selected bed
    Domain::BedRef selected_bed = m_project_interactor.scene_interactor().bed_selection().last_selected_bed();
    const auto& proj = m_workbench.project(m_project_interactor.selected_project_id());
    const Domain::ConfigContainer* cc = proj.find_config_container(selected_bed.config_container_id);
    DEBUG_ASSERT(cc != nullptr);
    const Domain::BedInstance& inst = cc->find_bed_instance(selected_bed.instance_id);
    Vec3d selected_bed_center = Biz::Algorithms::Point::to_3d(cc->bed().center(), 0.0) + inst.transformation.get_offset();
    scene().camera_trackball().set_target(selected_bed_center);
    scene().camera_trackball().synchronize_pivot_with_target();
}

static std::string bed_instance_thumbnail_name(size_t config_container_id, size_t instance_id)
{
    return fmt::format("thumbnail_bed_{}_{}", config_container_id, instance_id);
}

void PlaterScenePresenter::on_selected_project_changed(size_t index)
{
    if (index == Domain::INVALID_ID) {
        return;
    }

    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        m_projects.try_emplace(m_selected_project_id);
        m_bed_render_updater.on_selected_project_changed(m_selected_project_id);
        // a new camera has been created, add the bed updater as listener
        auto& camera = project_context().scene().camera();
        camera.add_listener<Scene::ICameraUpdateListener>(&m_bed_render_updater);
        camera.set_viewport(m_viewport);
    }

    m_camera_frustum_updater.update_scene_aabb(project_context().scene());
}

void PlaterScenePresenter::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection)
{
    m_volume_materials_dirty = true;

    bool selection_empty = selection.elements.empty();
    m_projects[project_id].selection_root().set_enabled(!selection_empty);

    if (selection_empty)
        return;

    update_selection_aabb(project_id, selection);
}

void PlaterScenePresenter::on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection)
{
    update_selection_aabb(project_id, selection);
}

void PlaterScenePresenter::on_selected_bed_instances_changed(Domain::SelectionId project_id, const Biz::Scene::BedSelection& selection)
{
    m_bed_render_updater.update_all(project_context().scene().camera());

    Eigen::AlignedBox3d bed_aabb;
    for (const auto& bed_instance : selected_bed_instances()) {
        const Domain::Vec3d bed_inst_offset{bed_instance.get().transformation.get_offset()};
        const std::vector<Domain::Vec3f> print_volume{
            Biz::Scene::BedGeometry::print_volume(bed_instance.get().bed.get())
        };
        for (const auto& v : print_volume) {
            bed_aabb.extend(bed_inst_offset + v.cast<double>());
        }
    }
    Scene::Scene::set_shadows_aabb(bed_aabb);
    m_volume_materials_dirty = true;
}

void
PlaterScenePresenter::build_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id, const Domain::ModelInstance* inst, const Domain::ModelVolume* vol, std::optional<ColorRGBA> color)
{
    SPDLOG_DEBUG("build_volume inst: {}  vol: {}", inst->id().id, vol->id().id);
    auto& ctx         = m_projects[project_id];
    auto& geom_mgr    = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();

    Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::Volume, vol->id().id};
    const auto& trimesh = trimesh_mgr.get_or_create(
        id,
        [&]() -> std::unique_ptr<Scene::TriangleMesh>
        { return std::make_unique<Scene::TriangleMesh>(vol->mesh_ptr()); }
    );
    const auto* geom = geom_mgr.get_or_create(
        id,
        [&]() { return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles()); }
    );

    ColorRGBA clr = color.has_value() ? *color : ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};
    if (!color.has_value()) {
        auto color_it = VOLUME_COLORS.find(vol->type());
        if (color_it != VOLUME_COLORS.end())
            clr = color_it->second;
    }

    if (!inst->printable)
        clr = saturate(clr, 0.25f);

    auto material =
        Render::Material{}
            .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
            .set_uniform("uniform_color", clr)
            .set_transparent(clr.is_transparent());
    builder.set_debug_name(fmt::format("vol: {}", vol->id().id))
        .transform([vol](auto& xform) { xform = vol->get_matrix(); })
        .set_tag(SceneNodeTag{vol->get_object()->id().id, vol->id().id, inst->id().id, vol->type()})
        .set_mesh(geom, material, Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects))
        .set_aabb(trimesh->aabb_mesh())
        // FIXME: for fff printers the pbr data should be set in dependence of the volume filament
        // see PrusaSlicer PrintConfigDef::init_fff_params() option 'filament_type'
        // and for sla printers it should be set in dependence of the resin type
        .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS);
}

PlaterScenePresenter::BedInstances PlaterScenePresenter::selected_bed_instances() const
{
    BedInstances result;
    const Biz::Scene::SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    const Biz::Scene::BedSelection& selection{scene_interactor.bed_selection()};
    const Domain::Project& project{m_workbench.project(m_project_interactor.selected_project_id())};
    const Domain::ConfigContainer& config_container{
        *ASSERT_VAL(project.find_config_container(selection.config_container_id()))
    };
    for (const auto& bed_instance : config_container.bed_instances()) {
        if (selection.is_selected(Domain::BedRef{config_container.id().id, bed_instance->id().id})) {
            result.push_back(*bed_instance);
        }
    }
    return result;
}

void PlaterScenePresenter::invoke_bed_visually_changed(Domain::SelectionId project_id)
{
    Domain::BedRefs bed_refs;
    const Domain::Project& project                  = m_workbench.project(project_id);
    const Domain::Project::ConfigContainerList& ccs = project.config_containers();
    for (const auto& cc : ccs) {
        for (const auto& bed_inst : cc->bed_instances()) {
            bed_refs.emplace_back(cc->id().id, bed_inst->id().id);
        }
    }

    if (!bed_refs.empty()) {
        invoke_listeners<Plater::IBedVisuallyChangedListener>(
            [&](Plater::IBedVisuallyChangedListener* l) { l->on_bed_changed(project_id, bed_refs); }
        );
    }
}

void PlaterScenePresenter::update_selection_aabb(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection)
{
    // update selection root, so it is in the center of all selected objects

    auto& proj              = m_projects[project_id];
    auto& selection_changes = project_context().selection_scene_changes();
    auto& scene             = proj.scene();

    Scene::Node::NodeList found_nodes;
    found_nodes.reserve(selection.elements.size());
    for (const auto& e : selection.elements) {
        scene.root().query(
            [&](const Scene::Node* n)
            {
                const auto* tag = n->tag_of_type<SceneNodeTag>();
                if (tag == nullptr)
                    return false;
                  return (selection.mode == Biz::Scene::SelectionMode::Instance) ?
                      tag->matches_element(e) : tag->object_id == e.object_id && tag->volume_id == e.volume_id;
            },
            found_nodes
        );
    }

    Eigen::AlignedBox3f bounds;
    for (const auto& n : found_nodes) {
        // visit all children to find all potential bounding boxes
        // this is important for instance-mode of selection where `n` itself has
        // no bounding box/raycast component
        visit(
            *n,
            [&](const Scene::Node& ni)
            {
                auto* collision = ni.raycast_component();
                if (collision != nullptr) {
                    auto wbb = collision->world_bounding_box(ni.world_transform().matrix());
                    for (size_t i = 0; i < 8; i++)
                        bounds.extend(wbb.corner(static_cast<decltype(wbb)::CornerType>(i)));
                }
            }
        );
    }

    proj.set_selection_bounding_box(bounds);
    if (!m_freeze_selection_center) {
        SquareMatrix4d xform = SquareMatrix4d::Identity();
        xform.col(3).head(3) = bounds.center().cast<double>();
        proj.selection_root().set_world_transform(Scene::Transform{xform});
    }
}

void PlaterScenePresenter::update_sinking_contours_visibility(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info)
{
    project_context().sinking_contours().update_visibility(e, screen_info, m_workbench.project(m_selected_project_id), scene());
}

void PlaterScenePresenter::on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    auto& scn                      = scene();
    const Domain::Project& project = m_workbench.project(project_id);

    Scene::NodeBuilder builder(scn);
    for (const auto& element : instances) {
        const Domain::ModelObject* obj = project.find_object_by_id(element.object_id);
        const Domain::ModelInstance*
            inst = Domain::find_by_id<Domain::ModelInstance>(obj->instances, element.instance_id);
        builder.set_debug_name(fmt::format("obj: {} inst: {}", obj->id().id, inst->id().id))
            .transform([inst](auto& t) { t = inst->get_matrix(); })
            .set_tag(SceneNodeTag{obj->id().id, 0, inst->id().id, Domain::ModelVolumeType::INVALID})
            .child_for_each(
                obj->volumes,
                [&](Scene::NodeBuilder& builder, const Domain::ModelVolume* vol)
                { build_volume_node(builder, project_id, inst, vol); }
            );
        scn.add_child(builder.build().release());
    }

    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, project, scn, instances);
    m_camera_frustum_updater.update_scene_aabb(scn);
}

void PlaterScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        instances,
        [](const auto& tag, const auto& el)
        { return tag.object_id == el.object_id && tag.instance_id == el.instance_id; }
    );

    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, m_workbench.project(project_id), scene(), instances);
    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements,
    Biz::Scene::TransformState state, const Biz::BedTrackingChanges& bed_tracking_changes)
{
    const BedInstances bed_instances{selected_bed_instances()};

    auto& scn        = scene();
    const auto& proj = m_workbench.project(project_id);
    Scene::visit(
        scn.root(),
        [&](Scene::Node& n)
        {
            const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
            if (t == nullptr)
                return;
            if (t->volume_id == 0) {
                for (const auto& e : elements) {
                    if (t->instance_id == e.instance_id) {
                        const auto* inst = proj.find_instance_by_id(e.object_id, e.instance_id);
                        n.set_local_transform(Scene::Transform{inst->get_matrix()});
                    }
                }
            }
        }
    );

    if (state == Biz::Scene::TransformState::Completed ||
        (!bed_tracking_changes.updated_beds.empty() && bed_tracking_changes.unplaced_instances_updated) || bed_tracking_changes.colliding_instances_updated_count != 0)
        m_volume_materials_dirty = true;

    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    auto& sinking_contours = project_context().sinking_contours();
    sinking_contours.update_scene(m_device, proj, scn, elements);
    sinking_contours.set_selection(elements);
    m_camera_frustum_updater.update_scene_aabb(scn);
}

void PlaterScenePresenter::on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    // find all instances of given object id and insert the volume node as child
    DEBUG_ASSERT(volumes.size() > 0);

    std::set<size_t> object_ids;
    for (const auto& v : volumes)
        object_ids.insert(v.object_id);
    auto& scn = scene();
    const Domain::Project& project = m_workbench.project(project_id);

    Scene::visit_conditional(scn.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t != nullptr && t->volume_id == 0 && object_ids.contains(t->object_id)) {
            // root of the instance
            const auto* obj = project.find_object_by_id(t->object_id);
            const auto* inst = Domain::find_by_id<Domain::ModelInstance>(obj->instances, t->instance_id);
            Scene::NodeBuilder builder{scn};
            for (const auto& e : volumes) {
                if (e.object_id != t->object_id)
                    continue;
                const auto* vol = Domain::find_by_id<Domain::ModelVolume>(obj->volumes, e.volume_id);
                build_volume_node(builder, project_id, inst, vol);
                scn.add_child(builder.build().release(), &n);
            }

            return false;
        }
        return true;
    });

    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, project, scn, volumes);
    m_camera_frustum_updater.update_scene_aabb(scn);
}

void PlaterScenePresenter::on_volume_removed(
    Domain::SelectionId project_id,
    const Domain::ElementRefs& volumes
)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        volumes,
        [](const SceneNodeTag& tag, const Domain::ElementRef& el) {
            return tag.object_id == el.object_id && tag.volume_id == el.volume_id;
        }
    );

    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, m_workbench.project(project_id), scene(), volumes);
    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state,
    const Biz::BedTrackingChanges& bed_tracking_changes)
{
    auto& scn        = scene();
    const auto& proj = m_workbench.project(project_id);

    Scene::visit(scn.root(), [&](Scene::Node& n) {
        const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
        if (t == nullptr || t->volume_id == 0)
            return;
        for (const auto& e : elements) {
            if (t->volume_id == e.volume_id) {
                const auto* vol = proj.find_volume_by_id(e.object_id, e.volume_id);
                n.set_local_transform(vol->get_matrix());
            }
        }
    });

    if (state == Biz::Scene::TransformState::Completed || (!bed_tracking_changes.updated_beds.empty() && bed_tracking_changes.unplaced_instances_updated))
        m_volume_materials_dirty = true;

    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    auto& sinking_contours = project_context().sinking_contours();
    sinking_contours.update_scene(m_device, proj, scn, elements);
    sinking_contours.set_selection(elements);
    m_camera_frustum_updater.update_scene_aabb(scn);
}

void PlaterScenePresenter::on_bed_instance_updated(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    auto& scn        = scene();
    const auto& proj = m_workbench.project(project_id);

    remove_beds(project_id, instances);

    for (auto& instance : instances) {
        const Domain::ConfigContainer* cc = proj.find_config_container(instance.config_container_id);
        DEBUG_ASSERT(cc != nullptr);
        const Domain::BedInstance& inst = cc->find_bed_instance(instance.instance_id);

        Scene::BedNodeTag tag = {instance.config_container_id, instance.instance_id};

        Scene::NodeBuilder builder(scn);
        Scene::BedNodeBuilder::bed_node(builder, inst, tag, m_device, m_projects[project_id], Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects));

        scn.add_child(builder.build().release());
    }

    m_bed_render_updater.update_all(scn.camera());
    m_volume_materials_dirty = true;
    m_camera_frustum_updater.update_scene_aabb(scn);

    invoke_bed_visually_changed(project_id);
}

void PlaterScenePresenter::on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    remove_beds(project_id, instances);

    m_bed_render_updater.update_all(scene().camera());
    m_camera_frustum_updater.update_scene_aabb(scene());

    m_volume_materials_dirty = true;
    invoke_bed_visually_changed(project_id);
}

void PlaterScenePresenter::on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances, Biz::Scene::TransformState state)
{
    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_wipe_tower_added(Domain::SelectionId project_id, Domain::SelectionId wipe_tower_id)
{
    invoke_bed_visually_changed(project_id);
    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_wipe_tower_removed(Domain::SelectionId project_id, Domain::SelectionId wipe_tower_id)
{
    invoke_bed_visually_changed(project_id);
    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_wipe_tower_transformed(Domain::SelectionId project_id, Domain::SelectionId wipe_tower_id, Biz::Scene::TransformState state)
{
    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    m_camera_frustum_updater.update_scene_aabb(scene());
}

void PlaterScenePresenter::on_layer_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx)
{
    cmd_buf.set_depth_write_enabled(true);
    if (layer_idx == Scene::RenderLayerId(PlaterSceneLayer::GizmoHandles))
        // clear depth buffer so all gizmo handles are rendered over document objects
        cmd_buf.clear_buffers(false, true);
    else if (layer_idx == Scene::RenderLayerId(PlaterSceneLayer::ObjectAccessoriesOnTop))
        cmd_buf.set_depth_test_enabled(false);
    else if (layer_idx == Scene::RenderLayerId(PlaterSceneLayer::AlwaysOnTop))
        // clear depth buffer to ensure geometry belonging to this layer is always rendered over any other object
        cmd_buf.clear_buffers(false, true);
}

void PlaterScenePresenter::on_opaque_pass_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx)
{
    MinimalSceneRenderCustomizer::on_opaque_pass_begin(cmd_buf, layer_idx);
    if (layer_idx == int(PlaterSceneLayer::ObjectAccessoriesOnTop))
        cmd_buf.set_depth_test_enabled(false);
}

void PlaterScenePresenter::on_transparent_pass_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx)
{
    MinimalSceneRenderCustomizer::on_transparent_pass_begin(cmd_buf, layer_idx);
    if (layer_idx == int(PlaterSceneLayer::ObjectAccessoriesOnTop))
        cmd_buf.set_depth_test_enabled(false);
}

void PlaterScenePresenter::remove_beds(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    remove_children<Scene::BedNodeTag, Domain::BedRef>(
        scene(),
        instances,
        [](const Scene::BedNodeTag& tag, const Domain::BedRef& br)
        {
            return tag.config_container_id == br.config_container_id && tag.instance_id == br.instance_id;
        }
    );

    std::set<std::size_t> active_beds;
    std::set<std::size_t> active_bed_instances;

    const Domain::Project& project{m_workbench.project(project_id)};
    for (const auto& config_container : project.config_containers()) {
        active_beds.insert(config_container->bed().id().id);
        for (const auto& bed_instance : config_container->bed_instances()) {
            active_bed_instances.insert(bed_instance->id().id);
        }
    }

    const auto is_active = [&](const Scene::AuxiliaryElementId& id)
    {
        using Type = Scene::AuxiliaryElementId::Type;
        switch (id.type) {
        case Type::BedLabel:
            return active_bed_instances.contains(id.id);
        case Type::BedPlate:
            [[fallthrough]];
        case Type::BedGrid:
            [[fallthrough]];
        case Type::BedContour:
            [[fallthrough]];
        case Type::BedPrintVolume:
            [[fallthrough]];
        case Type::BedAxis:
            [[fallthrough]];
        case Type::BedModel:
            return active_beds.contains(id.id);
        default:
            return true;
        }
    };

    Scene::ScenePresenterProjectContext& project_context{m_projects.at(project_id)};
    project_context.model_geometry_manager().release_if(
        [&](const Scene::AuxiliaryElementId& id, const Render::Geometry&) { return !is_active(id); }
    );

    project_context.model_triangle_mesh_manager().release_if(
        [&](const Scene::AuxiliaryElementId& id, const Scene::TriangleMesh&)
        { return !is_active(id); }
    );
}

} // namespace Slic3r::App::Plater
