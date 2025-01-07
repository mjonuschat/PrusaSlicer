#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/App/Plater/BedNodeTag.hpp"
#include "Slic3r/App/Plater/BedRenderHelper.hpp"
#include "Slic3r/Biz/Plater/BedGeometry.hpp"

#include "libslic3r/Model.hpp"

#define ENABLED_DEBUG_BEDS 1

#if ENABLED_DEBUG_BEDS
#include <imgui/imgui.h>
#endif // ENABLED_DEBUG_BEDS

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
    , m_bed_render_updater(*this)
{
//    std::for_each(m_workbench.projects().begin(), m_workbench.projects().end(), [this](const auto& p) {
//        m_projects.emplace(p.first, ScenePresenterProjectContext{});
//    });
    ScenePresenter::on_selected_project_changed(m_project_interactor.selected_project_id());
    project_context().scene().camera().add_update_listener(&m_bed_render_updater);
}

void ScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty())
        project_context().scene().render(command_buffer, this);
}

#if ENABLED_DEBUG_BEDS
static const std::vector<Vec3d> bed_offsets = {
    { 0.0,     0.0, 0.0 },
    { 300.0,   0.0, 0.0 },
    { 0.0,   300.0, 0.0 },
    { 300.0, 300.0, 0.0 },
    { 600.0,   0.0, 0.0 },
    { 600.0, 300.0, 0.0 },
    { 0.0,   600.0, 0.0 },
    { 300.0, 600.0, 0.0 },
    { 600.0, 600.0, 0.0 },
};

class MultipleBeds
{
public:
[[nodiscard]] static std::pair<size_t, size_t> active(const Domain::BedContainer& container)
{
    const Domain::BedContainer::BedList& beds = container.beds();
    for (size_t i = 0; i < beds.size(); ++i) {
        const Domain::Bed* bed = beds[i].get();
        const Domain::Bed::BedInstances& instances = bed->instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            const Domain::BedInstance* inst = instances[j];
            if (inst->active())
                return { bed->id().id, inst->id().id };
        }
    }
    return { 0, 0 };
}

static void set_active(Domain::BedContainer& container, size_t bed_idx, size_t instance_idx)
{
    Domain::BedContainer::BedList& beds = container.beds();
    for (size_t i = 0; i < beds.size(); ++i) {
        Domain::Bed* bed = beds[i].get();
        Domain::Bed::BedInstances& instances = bed->instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            Domain::BedInstance* inst = instances[j];
            inst->set_active(bed->id().id == bed_idx && inst->id().id == instance_idx);
        }
    }
}

static size_t instances_count(Domain::BedContainer& container)
{
    size_t ret = 0;
    Domain::BedContainer::BedList& beds = container.beds();
    for (size_t i = 0; i < beds.size(); ++i) {
        ret += beds[i]->instances().size();
    }
    return ret;
}

static void refresh_layout(Domain::BedContainer& container)
{
    Domain::BedContainer::BedList& beds = container.beds();
    size_t instances_count = 0;
    for (size_t i = 0; i < beds.size(); ++i) {
        Domain::Bed* bed = beds[i].get();
        Domain::Bed::BedInstances& instances = bed->instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            Transform3d bed_xform = Geometry::translation_transform(to_3d(-bed->center(), 0) + bed_offsets[instances_count]);
            instances[j]->set_transformation(Geometry::Transformation{ bed_xform });
            ++instances_count;
        }
    }
}
};
#endif // ENABLED_DEBUG_BEDS

void ScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
#if ENABLED_DEBUG_BEDS
    if (ImGui::Begin("Bed test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        Domain::Project& proj = const_cast<Domain::Project&>(m_workbench.project(m_selected_project_id));

        size_t instances_count = MultipleBeds::instances_count(proj.bed_container());
        BedNodeTag* remove_tag = nullptr;

        if (ImGui::BeginTable("Beds", (instances_count > 1) ? 6 : 5, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Bed ID");
            ImGui::TableSetupColumn("Instance ID");
            ImGui::TableSetupColumn("Active");
            ImGui::TableSetupColumn("Contour");
            ImGui::TableSetupColumn("Print Volume");
            ImGui::TableHeadersRow();

            Scene::visit(scene().root(), [&](Scene::Node& n) {
                BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
                if (tag != nullptr) {
                    if (tag->type == BedElementType::Undefined) {
                        Domain::Bed* bed = proj.bed_container().bed(tag->bed_id);
                        DEBUG_ASSERT(bed != nullptr);
                        Domain::BedInstance* inst = bed->instance(tag->instance_id);
                        DEBUG_ASSERT(inst != nullptr);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%zu", tag->bed_id);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%zu", tag->instance_id);
                        ImGui::TableSetColumnIndex(2);
                        bool active = inst->active();
                        if (ImGui::Checkbox(fmt::format("##active{}/{}", tag->bed_id, tag->instance_id).c_str(), &active)) {
                            if (active) {
                                MultipleBeds::set_active(proj.bed_container(), tag->bed_id, tag->instance_id);
                                m_bed_render_updater.update_materials(m_device, proj);
                            }
                        }
                        ImGui::TableSetColumnIndex(3);
                        bool contour = inst->contour_enabled();
                        if (ImGui::Checkbox(fmt::format("##contour{}/{}", tag->bed_id, tag->instance_id).c_str(), &contour)) {
                            inst->set_contour_enabled(contour);
                            m_bed_render_updater.update_elements_state(proj);
                        }
                        ImGui::TableSetColumnIndex(4);
                        bool print_volume = inst->print_volume_enabled();
                        if (ImGui::Checkbox(fmt::format("##print_volume{}/{}", tag->bed_id, tag->instance_id).c_str(), &print_volume)) {
                            inst->set_print_volume_enabled(print_volume);
                            m_bed_render_updater.update_elements_state(proj);
                        }

                        if (instances_count > 1) {
                            ImGui::TableSetColumnIndex(5);
                            if (ImGui::Button(fmt::format("Remove##{}/{}", tag->bed_id, tag->instance_id).c_str())) {
                                bed->remove_instance(tag->instance_id);

                                std::pair<size_t, size_t> active = MultipleBeds::active(proj.bed_container());
                                if (active.first == 0 && active.second == 0) {
                                    std::vector<size_t> beds_idxs = proj.bed_container().beds_indices();
                                    Domain::Bed* b = proj.bed_container().bed(beds_idxs.front());
                                    MultipleBeds::set_active(proj.bed_container(), b->id().id, b->instances().front()->id().id);
                                }

                                MultipleBeds::refresh_layout(proj.bed_container());
                                remove_tag = tag;
                            }
                        }
                    }
                }
            });

            ImGui::EndTable();
        }

        if (remove_tag != nullptr) {
            scene().remove_children([&](const auto* n) {
                const BedNodeTag* tag = n->template tag_of_type<BedNodeTag>();
                return tag != nullptr && tag->bed_id == remove_tag->bed_id && tag->instance_id == remove_tag->instance_id;
            });
            update_beds();
        }

        size_t total_instances_count = 0;
        std::vector<size_t> beds_idxs = proj.bed_container().beds_indices();
        for (size_t i : beds_idxs) {
            total_instances_count += proj.bed_container().bed(i)->instances().size();
        }

        if (total_instances_count < 9) {
            if (ImGui::Button("Add instance")) {
                Domain::Bed* b = proj.bed_container().bed(beds_idxs.front());
                Domain::BedInstance* i = b->add_instance();
                Transform3d bed_xform = Geometry::translation_transform(to_3d(-b->center(), 0) + bed_offsets[total_instances_count]);
                i->set_transformation(Geometry::Transformation{ bed_xform });
                const Domain::ElementRef updated{ b->id().id, i->id().id };
                on_bed_instance_added(m_selected_project_id, updated);
                MultipleBeds::set_active(proj.bed_container(), b->id().id, i->id().id);
                update_beds();
            }
        }

    }
    ImGui::End();
#endif // ENABLED_DEBUG_BEDS

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

    AuxiliaryElementId id{ AuxiliaryElementId::Type::Volume, vol->id().id};
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

void ScenePresenter::build_bed_plate_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id,
    const Domain::Bed& bed, const BedNodeTag& tag)
{
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();

    std::vector<std::pair<Vec3f, Vec2f>> triangles = Biz::Plater::BedGeometry::plate_triangles(bed);
    DEBUG_ASSERT(!triangles.empty());

    BedElementType type = bed.texture_filename().empty() ? BedElementType::PlateDefault : BedElementType::PlateTextured;
    AuxiliaryElementId id{ AuxiliaryElementId::Type::Bed, tag.bed_id * 100 + size_t(type) };
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_triangles(m_device, triangles);
    });

    auto material = Render::Material{};
    if (type == BedElementType::PlateDefault) {
        const Domain::BedInstance* inst = bed.instance(tag.instance_id);
        ColorRGBA color = inst->active() ? DEFAULT_BED_PLATE_COLOR : DISABLED_BED_PLATE_COLOR;
        material
            .set_shader(m_device.context().shader_manager().get_shader("flat"))
            .set_uniform("uniform_color", color)
            .set_transparent(color.a() < 1.0f);
    }
    else {
        material
            .set_shader(m_device.context().shader_manager().get_shader("printbed"))
            .set_uniform("transparent_background", true)
            .set_uniform("svg_source", boost::algorithm::iends_with(bed.texture_filename(), ".svg"));
    }

    builder
        .child([&](Scene::NodeBuilder& bldr) {
            bldr
                .set_debug_name(fmt::format("bed: {} plate", bed.id().id))
                .set_tag(BedNodeTag{ tag.bed_id, tag.instance_id, type })
                .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));
        });
}

void ScenePresenter::build_bed_grid_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id,
    const Domain::Bed& bed, const BedNodeTag& tag)
{
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();

    std::vector<Vec3f> lines = App::Plater::BedRenderHelper::plate_grid(bed);
    DEBUG_ASSERT(!lines.empty());

    AuxiliaryElementId id{ AuxiliaryElementId::Type::Bed, tag.bed_id * 100 + size_t(BedElementType::Grid) };
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_lines(m_device, lines);
    });

    ColorRGBA color = DEFAULT_BED_GRID_COLOR;

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);

    builder
        .child([&](Scene::NodeBuilder& bldr) {
            bldr
                .set_debug_name(fmt::format("bed: {} grid", bed.id().id))
                .set_tag(BedNodeTag{ tag.bed_id, tag.instance_id, BedElementType::Grid })
                .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));
        });
}

void ScenePresenter::build_bed_contour_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id,
    const Domain::Bed& bed, const BedNodeTag& tag)
{
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();

    std::vector<Vec3f> lines = Biz::Plater::BedGeometry::plate_contour(bed);
    DEBUG_ASSERT(!lines.empty());

    AuxiliaryElementId id{ AuxiliaryElementId::Type::Bed, tag.bed_id * 100 + size_t(BedElementType::Contour) };
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_lines(m_device, lines);
    });

    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);

    builder
        .child([&](Scene::NodeBuilder& bldr) {
            bldr
                .set_debug_name(fmt::format("bed: {} contour", bed.id().id))
                .set_tag(BedNodeTag{ tag.bed_id, tag.instance_id, BedElementType::Contour })
                .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));
        });
}

void ScenePresenter::build_bed_print_volume_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id,
    const Domain::Bed& bed, const BedNodeTag& tag)
{
    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();

    std::vector<Vec3f> lines = Biz::Plater::BedGeometry::print_volume(bed);
    DEBUG_ASSERT(!lines.empty());

    AuxiliaryElementId id{ AuxiliaryElementId::Type::Bed, tag.bed_id * 100 + size_t(BedElementType::PrintVolume) };
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_lines(m_device, lines);
    });

    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);

    builder
        .child([&](Scene::NodeBuilder& bldr) {
            bldr
                .set_debug_name(fmt::format("bed: {} contour", bed.id().id))
                .set_tag(BedNodeTag{ tag.bed_id, tag.instance_id, BedElementType::PrintVolume })
                .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));
        });
}

void ScenePresenter::build_bed_model_node(Scene::NodeBuilder& builder, Domain::SelectionId project_id,
    const Domain::Bed& bed, const BedNodeTag& tag)
{
    TriangleMesh mesh = Biz::Plater::BedGeometry::model(bed);
    if (mesh.empty()) {
        SPDLOG_ERROR("Found empty mesh");
        return;
    }

    auto& ctx = m_projects[project_id];
    auto& geom_mgr = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();

    AuxiliaryElementId id{ AuxiliaryElementId::Type::Bed, tag.bed_id * 100 + size_t(BedElementType::Model) };
    const auto& trimesh =
        trimesh_mgr.get_or_create(id, [&]() -> std::unique_ptr<Scene::TriangleMesh> {
            return std::make_unique<Scene::TriangleMesh>(std::move(mesh.its));
        });
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });

    ColorRGBA color = DEFAULT_BED_MODEL_COLOR;

    auto material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);

    builder
        .child([&](Scene::NodeBuilder& bldr) {
            bldr
                .set_debug_name(fmt::format("bed: {} model", bed.id().id))
                .set_tag(BedNodeTag{ tag.bed_id, tag.instance_id, BedElementType::Model })
                .set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));
        });
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

void ScenePresenter::on_bed_instance_added(Domain::SelectionId project_id, const Domain::ElementRef& instance)
{
    auto& scn = scene();
    const auto& proj = m_workbench.project(project_id);

    const Domain::Bed* bed = proj.bed_container().bed(instance.object_id);
    DEBUG_ASSERT(bed != nullptr);
    const Domain::BedInstance* inst = bed->instance(instance.instance_id);
    DEBUG_ASSERT(inst != nullptr);

    BedNodeTag tag = { instance.object_id, instance.instance_id };

    Scene::NodeBuilder builder(scn);
    builder
        .set_debug_name(fmt::format("bed: {} inst: {}", instance.object_id, instance.instance_id))
        .set_tag(tag)
        .transform([inst](auto& t) { t = inst->matrix(); });

    build_bed_plate_node(builder, project_id, *bed, tag);
    if (!bed->model_filename().empty())
        build_bed_model_node(builder, project_id, *bed, tag);
    if (bed->texture_filename().empty())
        build_bed_grid_node(builder, project_id, *bed, tag);
    build_bed_contour_node(builder, project_id, *bed, tag);
    build_bed_print_volume_node(builder, project_id, *bed, tag);

    scn.add_child(builder.build().release());
}

void ScenePresenter::on_bed_instance_removed(Domain::SelectionId project_id, const Domain::ElementRef& instance)
{
}

void ScenePresenter::on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRef& instance)
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
    cmd_buf.set_depth_write_enabled(true);
    if (layer_idx == int(PlaterSceneLayer::GizmoHandles))

        // clear depth buffer so all gizmo handles are rendered over document objects
        cmd_buf.clear_buffers(false, true);
}


} // namespace Slic3r::App::Plater
