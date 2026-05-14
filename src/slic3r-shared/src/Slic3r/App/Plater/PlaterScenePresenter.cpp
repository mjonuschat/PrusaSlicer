#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"

#include "Slic3r/Biz/IColorsChangedListener.hpp"

#include <ranges>
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Transformation.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedRenderHelper.hpp"
#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/App/Render/FramebufferManager.hpp"
#include "Slic3r/App/Scene/BedNodeBuilder.hpp"
#include "Slic3r/App/Plater/ThumbnailRenderer.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Scene/BedTracking.hpp"
#include "Slic3r/App/Scene/PrintVolumeData.hpp"
#include "Slic3r/App/Scene/CameraHelper.hpp"
#include "Slic3r/App/Scene/VolumeColor.hpp"
#include "Slic3r/Math.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"

#include <libslic3r/GCode/WipeTower.hpp>

#define ENABLE_DEBUG_OBJECT_SELECTION 0
#define ENABLE_DEBUG_HOVER 0

using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::SquareMatrix3d;
using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Vec3d;

using Slic3r::Biz::Algorithms::Color::saturate;

using Slic3r::App::Scene::SceneNodeTag;

namespace Slic3r::App::Plater {

static void update_printable_color(ColorRGBA& inout_color, bool is_printable)
{
    if (is_printable)
        return;
    inout_color = saturate(inout_color, 0.25f);
}

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

std::optional<ColorRGBA> color_from_extruder_slot(
    const std::vector<Domain::ColorRGB>& slot_colors,
    const Domain::ModelVolume& vol,
    bool is_printable
)
{
    const int raw_id = vol.extruder_id();
    const int slot   = (raw_id <= 0) ? 0 : raw_id - 1;
    if (slot < static_cast<int>(slot_colors.size())) {
        const auto& c = slot_colors[slot];
        ColorRGBA color{c.r(), c.g(), c.b(), 1.0f};
        update_printable_color(color, is_printable);
        return color;
    }
    return std::nullopt;
}

} // namespace

PlaterScenePresenter::PlaterScenePresenter(
    const Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor,
    Render::Device& device,
    Platform::AnimationManager& animation_manager
) :
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_device(device),
    m_bed_render_updater(*this, workbench, device, project_interactor.scene_interactor()),
    m_animation_manager(animation_manager),
    m_data_factory(device, "plater_scene_presenter")
{
    load_selected_project();

    m_project_interactor.add_listener<ISelectedProjectChangedListener>(&m_bed_render_updater);
    m_project_interactor.add_listener<ISelectedProjectChangedListener>(this);
    m_project_interactor.add_listener<Biz::IProjectsChangedListener>(this);

    m_project_interactor.project_settings_interactor()
        .add_listener<Biz::IColorsChangedListener>(this);

    auto& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.add_listener<Biz::Scene::ISceneChangedListener>(this);
    scene_interactor.add_listener<ISceneBedInstanceChangedListener>(this);
    scene_interactor.add_listener<ISceneSelectionChangedListener>(this);
    scene_interactor.add_listener<ISelectedBedInstancesChangedListener>(this);
}

void PlaterScenePresenter::load_selected_project()
{
    size_t project_id = m_project_interactor.selected_project_id();
    PlaterScenePresenter::on_selected_project_changed(project_id);
    m_bed_render_updater.on_selected_project_changed(project_id);

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

    center_camera_on_selected_bed(false);
}

void PlaterScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty()){
        if (m_volume_materials_dirty) {
            update_volume_materials();
            m_volume_materials_dirty = false;
        }

        project_context().update_selection_obb_node(m_device, m_project_interactor);
#if ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_scene_aabb(project_context());
        m_camera_frustum_updater.update_scene_aabb_node(project_context(), m_device);
#else
        m_camera_frustum_updater.update_scene_aabb(scene());
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_camera_frustum(scene().camera());
        scene().render(m_device, command_buffer, this);
    }
}

#if ENABLE_DEBUG_OBJECT_SELECTION
void render_imgui_debug_object_selection(const Biz::Scene::ObjectSelection& selection)
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Object selection debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Selection");
        if (ImGui::BeginTable("Selection", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Mode");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", (selection.mode == Biz::Scene::SelectionMode::Instance) ? "Instance" : "Volume");

            if (selection.elements.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Empty");
            }
            else {
                for (size_t i = 0; i < selection.elements.size(); ++i) {
                    const Domain::ElementRef& e = selection.elements[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Element %zu", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%zu, %zu, %zu", e.object_id, e.instance_id, e.volume_id);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}
#endif // ENABLE_DEBUG_OBJECT_SELECTION

#if ENABLE_DEBUG_HOVER
void render_imgui_debug_hover(const HoverData& hover_data)
{
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Hover debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Hover");
        if (ImGui::BeginTable("Hover", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Mode");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", (hover_data.type == HoverType::Select) ? "Select" :
                              (hover_data.type == HoverType::Unselect) ? "Unselect" : "None");

            if (hover_data.nodes.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Empty");
            }
            else {
                for (size_t i = 0; i < hover_data.nodes.size(); ++i) {
                    const Scene::Node* n = hover_data.nodes[i];
                    const SceneNodeTag* tag = n->tag_of_type<SceneNodeTag>();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Node %zu", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%zu, %zu, %zu", tag->object_id, tag->instance_id, tag->volume_id);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}
#endif // ENABLE_DEBUG_HOVER

void PlaterScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty()) {
#if ENABLE_DEBUG_OBJECT_SELECTION
        render_imgui_debug_object_selection(m_project_interactor.scene_interactor().object_selection());
#endif // ENABLE_DEBUG_OBJECT_SELECTION
#if ENABLE_DEBUG_HOVER
        render_imgui_debug_hover(m_hover_data);
#endif // ENABLE_DEBUG_HOVER
#if ENABLE_DEBUG_BED_ERROR
        render_imgui_debug_bed_error(project_context().bed_error());
#endif // ENABLE_DEBUG_BED_ERROR
        scene().render_imgui(screen_info);
    }
}

void PlaterScenePresenter::screen_resized(const Render::Rect& viewport)
{
    m_viewport = viewport;
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PlaterScenePresenter::on_hover_changed(const HoverData& hover_data)
{
    m_hover_data = hover_data;
    m_volume_materials_dirty = true;
}

void PlaterScenePresenter::on_node_added(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PlaterScenePresenter::on_node_removed(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PlaterScenePresenter::on_node_changed(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PlaterScenePresenter::camera_updated(const Scene::Camera& cam)
{
    set_scene_aabb_as_dirty();
    invoke_listeners<ICameraUpdateListener>([&cam](auto* l) { l->camera_updated(cam); });
}

void PlaterScenePresenter::on_project_loaded(Domain::SelectionId project_id)
{
    center_camera_on_selected_bed(false);
}

void PlaterScenePresenter::on_fdm_result_cache_changed(const Domain::SlicingId id)
{
    std::optional<Biz::FDMResultRef> fdm_result{m_project_interactor.fdm_result_cache().get_result(id)};
    update_bed_instance_error_state(id, fdm_result.has_value() && !fdm_result->get().contained_in_bed);
}

void PlaterScenePresenter::on_sla_result_cache_changed(const Domain::SlicingId& id)
{
    std::optional<Biz::SLAResultRef> sla_result{m_project_interactor.sla_result_cache().get_result(id)};
    update_bed_instance_error_state(id, sla_result.has_value() && !sla_result->get().contained_in_bed);
}

void PlaterScenePresenter::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId /*config_container_id*/,
    const std::vector<Domain::ColorRGB>& /*colors*/
)
{
    m_volume_materials_dirty = true;
    invoke_bed_visually_changed(project_id);
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

static std::unordered_map<Domain::SelectionId, Domain::SelectionId> model_instance_to_config_container_map(const Domain::Project& project)
{
    std::unordered_map<Domain::SelectionId, Domain::SelectionId> ret;
    for (const auto& cc : project.config_containers()) {
        for (const auto& bi : cc->bed_instances()) {
            for (const auto* mi : bi->model_instances) {
                ret[mi->id().id] = cc->id().id;
            }
        }
    }
    return ret;
}

static std::optional<ColorRGBA> select_color(
    bool is_model_part,
    bool is_selected,
    bool is_outside,
    bool is_disabled,
    bool is_printable,
    HoverType hover_type
)
{
    static const ColorRGBA OUTSIDE_COLOR          = ColorRGBA(0.0f, 0.38f, 0.8f, 1.0f);
    static const ColorRGBA OUTSIDE_SELECTED_COLOR = ColorRGBA(0.19f, 0.58f, 1.0f, 1.0f);
    static const ColorRGBA SELECTED_COLOR         = ColorRGBA::GREEN();// ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
    static const ColorRGBA HOVER_SELECT_COLOR     = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f);
    static const ColorRGBA HOVER_UNSELECT_COLOR   = ColorRGBA(1.0f, 0.75f, 0.75f, 1.0f);
    static const ColorRGBA DISABLED_COLOR         = ColorRGBA(0.25f, 0.25f, 0.25f, 1.0f);

    std::optional<ColorRGBA> ret;
    if (is_disabled)
        ret = DISABLED_COLOR;
    else if (is_selected && hover_type == HoverType::Unselect)
        ret = HOVER_UNSELECT_COLOR;
    else if (is_outside && is_selected)
        ret = OUTSIDE_SELECTED_COLOR;
    else if (is_outside && hover_type == HoverType::Select)
        ret = OUTSIDE_SELECTED_COLOR;
    else if (!is_selected && hover_type == HoverType::Select)
        ret = HOVER_SELECT_COLOR;
    else if (is_outside)
        ret = OUTSIDE_COLOR;
    else if (is_selected)
        ret = SELECTED_COLOR;

    if (ret.has_value() && !is_model_part)
        ret->a(0.65f);

    if (ret) {
        update_printable_color(*ret, is_printable);
    }

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
    std::unordered_map<Domain::SelectionId, Domain::SelectionId> mi_to_cc_map = model_instance_to_config_container_map(proj);

    Scene::visit(
        scene().root(),
        [&](Scene::Node& n)
        {
            const SceneNodeTag* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr && n.has_render_component()) {
                const auto* inst = proj.find_instance_by_id(tag->object_id, tag->instance_id);
                bool is_wipe_tower = tag->is_wipe_tower();
                bool is_on_bed = std::find(instances.first.begin(), instances.first.end(), inst) != instances.first.end() ||
                    std::find(instances.second.begin(), instances.second.end(), inst) != instances.second.end();
                bool is_on_selected_bed = is_wipe_tower ||
                    std::find(sel_instances.first.begin(), sel_instances.first.end(), inst) != sel_instances.first.end();
                bool is_model_part = tag->volume_type == Domain::ModelVolumeType::MODEL_PART;
                n.render_component()->set_shadows(((is_model_part || is_wipe_tower) && is_on_selected_bed) ?
                    Render::Shadows{true, true} : Render::Shadows{false, false}
                );

                const Domain::BedInstance* bed_inst = nullptr;
                const Domain::Bed* bed = nullptr;
                bool is_colliding = false;

                bool is_selected = is_wipe_tower ?
                    selection.is_selected(Domain::ElementRef{tag->wipe_tower_id}) :
                    selection.is_selected({tag->object_id, tag->instance_id, tag->volume_id});

                bool is_disabled = (selection.mode == Biz::Scene::SelectionMode::Volume && !selection.empty()) ?
                    tag->instance_id != selection.elements.front().instance_id : false;
                bool is_hovered = std::find_if(m_hover_data.nodes.begin(), m_hover_data.nodes.end(),
                    [&](const Scene::Node* h) { return h == &n; }) != m_hover_data.nodes.end();
                HoverType hover_type = is_hovered ? m_hover_data.type : HoverType::None;

                std::optional<ColorRGBA> color;
                if (is_wipe_tower) {
                    const Domain::BedInstance* wipe_tower_bed_instance =
                        proj.find_bed_instance_by_id(tag->wipe_tower_id.bed_instance_id);
                    const bool wipe_tower_is_outside =
                        (wipe_tower_bed_instance != nullptr
                         && wipe_tower_bed_instance->wipe_tower_is_outside);
                    color =
                        select_color(true, is_selected, wipe_tower_is_outside, false, true, hover_type);
                } else if (is_on_bed) {
                    bed_inst = find_bed_instance_by_model_instance_id(mi_to_bi_map, inst->id().id);
                    if (bed_inst == nullptr) {
                        bed_inst =
                            find_bed_instance_by_model_instance_id(ci_to_bi_map, inst->id().id);
                        is_colliding = true;
                    }
                    bed = &bed_inst->bed.get();

                    color = select_color(
                        is_model_part,
                        is_selected,
                        is_colliding,
                        is_disabled,
                        inst->printable,
                        hover_type
                    );
                } else {
                    color = select_color(is_model_part, is_selected, true, is_disabled, inst->printable, hover_type);
                }

                if (!color.has_value())
                    n.remove_material_override();
                else {
                    Render::Material mat = Render::Material{}.set_uniform("uniform_color", *color).set_transparent(color->is_transparent());
                    n.set_material_override(mat);
                }

                Scene::PrintVolumeData print_volume;
                if (bed != nullptr) {
                    Domain::Vec2d offset = Biz::Algorithms::Point::to_2d(bed_inst->transformation.get_offset());
                    print_volume.type = bed->type();
                    print_volume.z_data = is_model_part ? Domain::Vec2f(float(Scene::BED_OFFSET_Z), bed->max_print_height()) : Domain::Vec2f(-FLT_MAX, FLT_MAX);
                    if (print_volume.type == Domain::BedType::Circle) {
                        const Domain::Vec2d& center = bed->center();
                        double radius = 0.5 * bed->contour_aabb_extent().x();
                        print_volume.xy_data = Domain::Vec4f(
                            float(offset.x() + center.x()),
                            float(offset.y() + center.y()),
                            is_model_part ? float(radius) : FLT_MAX,
                            FLT_MAX
                        );
                    }
                    else {
                        const BoundingBoxf& aabb = bed->contour_aabb();
                        print_volume.xy_data = Domain::Vec4f(
                            is_model_part ? float(offset.x() + aabb.min.x()) : -FLT_MAX,
                            is_model_part ? float(offset.y() + aabb.min.y()) : -FLT_MAX,
                            is_model_part ? float(offset.x() + aabb.max.x()) : FLT_MAX,
                            is_model_part ? float(offset.y() + aabb.max.y()) : FLT_MAX
                        );
                    }
                }
                else {
                    print_volume.type = Domain::BedType::Invalid;
                    print_volume.z_data = Domain::Vec2f(-FLT_MAX, FLT_MAX);
                    print_volume.xy_data = Domain::Vec4f(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
                }

                std::optional<ColorRGBA> part_color;
                if (is_model_part && !is_wipe_tower && inst != nullptr) {
                    auto cc_it = mi_to_cc_map.find(tag->instance_id);
                    if (cc_it != mi_to_cc_map.end()) {
                        const auto& slot_colors = m_project_interactor
                            .project_settings_interactor().get_colors(cc_it->second);
                        const auto* obj = proj.find_object_by_id(tag->object_id);
                        const auto* vol = obj
                            ? Domain::find_by_id<Domain::ModelVolume>(obj->volumes, tag->volume_id)
                            : nullptr;
                        if (vol) {
                            part_color = color_from_extruder_slot(slot_colors, *vol, inst->printable);
                        }
                    }
                }

                Render::Material mat = n.render_component()->material();
                set_uniforms(print_volume, mat);
                if (part_color)
                    mat.set_uniform("uniform_color", *part_color);
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

void PlaterScenePresenter::center_camera_on_selected_bed(bool animated)
{
    if (animated)
        animated_center_camera_on_bed(m_workbench.project(m_project_interactor.selected_project_id()),
            m_project_interactor.scene_interactor().bed_selection().last_selected_bed(), scene().camera_trackball(),
            m_animation_manager);
    else
        center_camera_on_bed(m_workbench.project(m_project_interactor.selected_project_id()),
            m_project_interactor.scene_interactor().bed_selection().last_selected_bed(), scene().camera_trackball());
}

void PlaterScenePresenter::on_selected_project_changed(size_t index)
{
    if (index == Domain::INVALID_ID) {
        return;
    }

    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        m_projects.try_emplace(m_selected_project_id);
        std::shared_ptr<Scene::ModelGeometryProvider> shared_model_geometry_provider =
            std::make_shared<Scene::ModelGeometryProvider>("plater");
        project_context().set_model_geometry_provider(shared_model_geometry_provider);
        // a new camera has been created, add the camera update listeners
        auto& camera = project_context().scene().camera();
        camera.add_listener<Scene::ICameraUpdateListener>(&m_bed_render_updater);
        camera.add_listener<Scene::ICameraUpdateListener>(this);
        camera.set_viewport(m_viewport);
        project_context().scene().add_listener<App::Scene::ISceneChangedListener>(this);
    }
    set_scene_aabb_as_dirty();
}

void PlaterScenePresenter::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    m_volume_materials_dirty = true;
}

void PlaterScenePresenter::on_scene_selection_bounding_box_updated(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    update_selection_root(project_id, selection);
}

void PlaterScenePresenter::on_selected_bed_instances_changed(Domain::SelectionId project_id, const Biz::Scene::BedSelection& selection)
{
    update_bed_instances();
    m_volume_materials_dirty = true;
    if (selection.camera_action_on_selection() == Biz::Scene::CameraActionOnBedSelection::CenterOnBed)
        center_camera_on_selected_bed(true);
}


void PlaterScenePresenter::build_volume_node(
    Scene::NodeBuilder& builder,
    Domain::SelectionId project_id,
    const Domain::ModelInstance* inst,
    const Domain::ModelVolume* vol,
    std::optional<ColorRGBA> color
)
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
        auto color_it = Scene::VOLUME_COLORS.find(vol->type());
        if (color_it != Scene::VOLUME_COLORS.end())
            clr = color_it->second;
    }
    update_printable_color(clr, inst->printable);

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

void
PlaterScenePresenter::clear_orphan_volumes_from_managers(Domain::SelectionId project_id)
{
    auto& ctx         = m_projects[project_id];
    auto& geom_mgr    = ctx.model_geometry_manager();
    auto& trimesh_mgr = ctx.model_triangle_mesh_manager();

    std::set<size_t> volume_ids;
    visit(
        ctx.scene().root(),
        [&volume_ids](const Scene::Node& node)
        {
            if (const auto* tag{node.tag_of_type<SceneNodeTag>()};
                tag != nullptr && tag->volume_id != 0)
            {
                volume_ids.insert(tag->volume_id);
            }
        },
        false
    );

    const auto predicate = [&](Scene::AuxiliaryElementId id, const auto&)
    { return id.type == Scene::AuxiliaryElementId::Type::Volume && !volume_ids.contains(id.id); };

    trimesh_mgr.release_if(predicate);
    geom_mgr.release_if(predicate);
}

const double wipe_tower_brim_height{0.2};

static Scene::SceneNodeTag wipe_tower_tag(Domain::SlicingId id) {
    return Scene::SceneNodeTag{0, 0, 0, Domain::ModelVolumeType::INVALID, id};
}

void build_wipe_tower_cube(
    Scene::NodeBuilder& builder,
    const std::string& debug_name,
    Scene::GeometryDataFactory& data_factory,
    const Render::Material& material,
    double bottom_z,
    const Vec3d& scale,
    Domain::SlicingId slicing_id
)
{
    builder.child(
        [&](Scene::NodeBuilder& builder)
        {
            auto geom = data_factory.geometry(Scene::GeometryDataId::Cube);
            auto mesh = data_factory.triangle_mesh(Scene::GeometryDataId::Cube);

            builder.set_debug_name(debug_name)
                .set_material_override(material)
                .set_tag(wipe_tower_tag(slicing_id))
                .set_mesh(geom, material, Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects))
                .set_shadows(Render::Shadows{true, true})
                .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS)
                .set_aabb(mesh->aabb_mesh());

            Domain::Transform3d transform{Domain::Transform3d::Identity()};
            transform.translate(Vec3d{ 0.0, 0.0, bottom_z + 0.5 * scale.z() });
            transform.scale(scale);
            builder.set_transform(transform);
        }
    );
}

static Vec3d wipe_tower_offset(
    double wipe_tower_width,
    double wipe_tower_depth
)
{
     return {wipe_tower_width / 2.0, wipe_tower_depth / 2.0, 0.0};
}

void build_wipe_tower_cone(
    Scene::NodeBuilder& builder,
    Scene::GeometryDataFactory& data_factory,
    const Render::Material& material,
    double radius,
    double scale_x,
    const Vec3d& sizes, // x = width, y = depth, z = height
    double brim_width,
    Domain::SlicingId slicing_id
)
{
    if (radius <= 0) {
        return;
    }

    const double diameter{2.0 * radius};
    const Vec3d cone_scale{ diameter / scale_x, diameter, sizes.z() };

    if (diameter / scale_x <= sizes.y()) {
        // cone contained into the main block
        return;
    }

    builder.child(
        [&](Scene::NodeBuilder& builder)
        {
            auto geom = data_factory.geometry(Scene::GeometryDataId::Cone);
            auto mesh = data_factory.triangle_mesh(Scene::GeometryDataId::Cone);

            builder.set_debug_name("wipe_tower_cone")
                .set_material_override(material)
                .set_tag(wipe_tower_tag(slicing_id))
                .set_mesh(
                    geom,
                    material,
                    Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects)
                )
                .set_shadows(Render::Shadows{true, true})
                .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS)
                .set_aabb(mesh->aabb_mesh());

            Domain::Transform3d transform{Domain::Transform3d::Identity()};
            transform.scale(cone_scale);
            builder.set_transform(transform);
        }
    );

    const Vec3d brim_scale{
        diameter / scale_x + 2.0 * brim_width,
        diameter + 2.0 * brim_width,
        wipe_tower_brim_height
    };

    builder.child(
        [&](Scene::NodeBuilder& builder)
        {
            auto geom = data_factory.geometry(Scene::GeometryDataId::Cylinder);
            auto mesh = data_factory.triangle_mesh(Scene::GeometryDataId::Cylinder);

            builder.set_debug_name("wipe_tower_cone_brim")
                .set_material_override(material)
                .set_tag(wipe_tower_tag(slicing_id))
                .set_mesh(
                    geom,
                    material,
                    Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects)
                )
                .set_shadows(Render::Shadows{true, true})
                .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS)
                .set_aabb(mesh->aabb_mesh());
            Domain::Transform3d transform{Domain::Transform3d::Identity()};
            transform.scale(brim_scale);
            builder.set_transform(transform);
        }
    );
}

void PlaterScenePresenter::build_unknown_wipe_tower_node(
    Scene::NodeBuilder& builder,
    const Biz::Print::WipeTowerGeometry& wipe_tower,
    Domain::SlicingId slicing_id
)
{
    auto color{Domain::ColorRGBA(0.9f, 0.6f, 0.0f, 1.0f)};
    const Render::Material material{
        Render::Material{}
            .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
            .set_uniform("uniform_color", color)
    };

    const double height{wipe_tower.fallback_height};
    const double depth{wipe_tower.fallback_depth};
    const double width{wipe_tower.width};

    const Vec3d offset{ wipe_tower_offset(width, depth) };

    builder.child(
        [&](Scene::NodeBuilder& builder)
        {
            builder.set_debug_name("wipe_tower_main");
            Domain::Transform3d transform{Domain::Transform3d::Identity()};
            transform.translate(offset);
            builder.set_transform(transform).set_tag(wipe_tower_tag(slicing_id));

            build_wipe_tower_cube(
                builder,
                "wipe_tower_brim",
                m_data_factory,
                material,
                0.0,
                Vec3d{
                    width + 2 * wipe_tower.brim_width,
                    depth + 2 * wipe_tower.brim_width,
                    wipe_tower_brim_height
                },
                slicing_id
            );

            build_wipe_tower_cube(
                builder,
                "wipe_tower_body",
                m_data_factory,
                material,
                0.0,
                Vec3d{width, depth, height},
                slicing_id
            );

            build_wipe_tower_cone(
                builder,
                m_data_factory,
                material,
                wipe_tower.cone_radius,
                wipe_tower.cone_x_scale,
                Vec3d(width, depth, height),
                wipe_tower.brim_width,
                slicing_id
            );
        }
    );
}

void PlaterScenePresenter::build_wipe_tower_node(
    Scene::NodeBuilder& builder,
    const Biz::Print::WipeTowerGeometry& wipe_tower,
    Domain::SlicingId slicing_id
)
{
    ASSERT(!wipe_tower.depths.empty());
    ASSERT(wipe_tower.depths.front().z == 0.0);
    const Render::Material material{
        Render::Material{}
            .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
            .set_uniform("uniform_color", Domain::ColorRGBA::DARK_YELLOW())
    };

    using Biz::Print::ZDepth;

    const double depth{wipe_tower.depths.front().depth};

    const Vec3d offset{
        wipe_tower_offset(wipe_tower.width, depth)
    };

    builder.child(
        [&](Scene::NodeBuilder& builder)
        {
            Domain::Transform3d transform{Domain::Transform3d::Identity()};
            transform.translate(offset);
            builder.set_transform(transform).set_tag(wipe_tower_tag(slicing_id));

            build_wipe_tower_cube(
                builder,
                "wipe_tower_brim",
                m_data_factory,
                material,
                0.0,
                Vec3d{
                    wipe_tower.width + 2 * wipe_tower.brim_width,
                    wipe_tower.depths.front().depth + 2 * wipe_tower.brim_width,
                    wipe_tower_brim_height
                },
                slicing_id
            );

            for (size_t i{1}; i < wipe_tower.depths.size(); ++i) {
                const double next_z{wipe_tower.depths[i].z};
                const auto [z, depth]{wipe_tower.depths[i - 1]};
                build_wipe_tower_cube(
                    builder,
                    "wipe_tower_body",
                    m_data_factory,
                    material,
                    z,
                    Vec3d{wipe_tower.width, depth, next_z - z},
                    slicing_id
                );
            }

            build_wipe_tower_cone(
                builder,
                m_data_factory,
                material,
                wipe_tower.cone_radius,
                wipe_tower.cone_x_scale,
                Vec3d(wipe_tower.width, depth, wipe_tower.depths.back().z),
                wipe_tower.brim_width,
                slicing_id
            );
        }
    );
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
            [&](Plater::IBedVisuallyChangedListener* l) { l->on_bed_changed(project_id, bed_refs, project_context().bed_error()); }
        );
    }
}

void PlaterScenePresenter::update_selection_root(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
) {
    using Biz::Scene::SelectionReferenceFrame;

    PlaterScenePresenterProjectContext& project{m_projects[project_id]};

    project.selection_root.set_enabled(false);
    project.plain_selection_root.set_enabled(false);
    project.set_selection_obb_node_as_dirty();

    const std::optional<Biz::Scene::SelectionExtents> bounding_box{
        m_project_interactor.scene_interactor().selection_bounding_box()
    };

    if (!bounding_box) {
        invoke_listeners<ISelectionExtentsChangedListener>(
            [&](ISelectionExtentsChangedListener* l)
            { l->on_scene_selection_bounding_box_changed(project_id, bounding_box); }
        );
        return;
    }

    m_project_interactor.scene_interactor().reload_object_selection_reference_frame(
        m_project_interactor.scene_interactor().object_selection_reference_frame()
    );

    Scene::Transform transform{SquareMatrix4d::Identity()};
    transform.translate(bounding_box->oriented_bounding_box().center);
    transform.rotate(bounding_box->oriented_bounding_box().rotation);
    project.selection_root.set_world_transform(transform);
    project.selection_root.set_enabled(true);
    project.plain_selection_root.set_world_transform(transform);
    project.plain_selection_root.set_enabled(true);

    invoke_listeners<ISelectionExtentsChangedListener>(
        [&](ISelectionExtentsChangedListener* l)
        { l->on_scene_selection_bounding_box_changed(project_id, bounding_box); }
    );
}

void PlaterScenePresenter::clear_selection_root_children() {
    Scene::ScenePresenterProjectContext& project{project_context()};

    project.scene().remove_children(
        [](const Scene::Node*) { return true; },
        &project.selection_root
    );
    project.scene().remove_children(
        [](const Scene::Node*) { return true; },
        &project.plain_selection_root
    );
}

bool PlaterScenePresenter::update_bed_instance_error_state(const Domain::SlicingId& id, bool error)
{
    bool ret = error ? project_context().bed_error().add_bed_instance(id) : project_context().bed_error().remove_bed_instance(id);
    if (ret) {
        update_bed_instances();
        invoke_bed_visually_changed(m_selected_project_id);
    }
    return ret;
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
}

void PlaterScenePresenter::on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        instances,
        [](const auto& tag, const auto& el)
        { return tag.object_id == el.object_id && tag.instance_id == el.instance_id; }
    );

    clear_orphan_volumes_from_managers(project_id);
    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, m_workbench.project(project_id), scene(), instances);
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
                    if (e.is_wipe_tower()) {
                        continue;
                    }
                    if (t->instance_id != 0 && t->instance_id == e.instance_id) {
                        const auto* inst = proj.find_instance_by_id(e.object_id, e.instance_id);
                        ASSERT(inst);
                        n.set_local_transform(Scene::Transform{inst->get_matrix()});
                    }
                }
            }
        },
        state != Biz::Scene::TransformState::InProgress
    );

    if (state == Biz::Scene::TransformState::Completed ||
        (!bed_tracking_changes.updated_beds.empty() && bed_tracking_changes.unplaced_instances_updated) || bed_tracking_changes.colliding_instances_updated_count != 0)
        m_volume_materials_dirty = true;

    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    auto& sinking_contours = project_context().sinking_contours();
    sinking_contours.update_scene(m_device, proj, scn, elements);
    sinking_contours.set_selection(elements);
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
}

void PlaterScenePresenter::on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    remove_children<SceneNodeTag, Domain::ElementRef>(
        scene(),
        volumes,
        [](const SceneNodeTag& tag, const Domain::ElementRef& el) {
            if (el.has_instance() && tag.instance_id != el.instance_id) {
                return false;
            }
            return tag.object_id == el.object_id && tag.volume_id == el.volume_id;
        }
    );

    clear_orphan_volumes_from_managers(project_id);
    invoke_bed_visually_changed(project_id);
    project_context().sinking_contours().update_scene(m_device, m_workbench.project(project_id), scene(), volumes);
}

void PlaterScenePresenter::on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, Biz::Scene::TransformState state,
    const Biz::BedTrackingChanges& bed_tracking_changes)
{
    auto& scn        = scene();
    const auto& proj = m_workbench.project(project_id);

    Scene::visit(
        scn.root(),
        [&](Scene::Node& n)
        {
            const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
            if (t == nullptr || t->volume_id == 0)
                return;
            for (const auto& e : elements) {
                if (t->volume_id == e.volume_id) {
                    const auto* vol = proj.find_volume_by_id(e.object_id, e.volume_id);
                    n.set_local_transform(vol->get_matrix());
                }
            }
        },
        true
    );

    if (state == Biz::Scene::TransformState::Completed || (!bed_tracking_changes.updated_beds.empty() && bed_tracking_changes.unplaced_instances_updated))
        m_volume_materials_dirty = true;

    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);

    auto& sinking_contours = project_context().sinking_contours();
    sinking_contours.update_scene(m_device, proj, scn, elements);
    sinking_contours.set_selection(elements);
}

void PlaterScenePresenter::on_volume_type_changed(
    Domain::SelectionId project_id,
    const Domain::ElementRefs& volumes
)
{
    auto& scn        = scene();
    const auto& proj = m_workbench.project(project_id);

    Scene::visit(
        scn.root(),
        [&](Scene::Node& n)
        {
            const SceneNodeTag* t = n.tag_of_type<SceneNodeTag>();
            if (t == nullptr || t->volume_id == 0)
                return;
            for (const Domain::ElementRef& volume_el : volumes) {
                if (volume_el.volume_id != t->volume_id) {
                    continue;
                }

                const Domain::ModelVolume* model_volume =
                    proj.find_volume_by_id(volume_el.object_id, volume_el.volume_id);
                if (t->volume_type != model_volume->type()) {
                    SceneNodeTag new_tag(
                        t->object_id,
                        t->volume_id,
                        t->instance_id,
                        model_volume->type(),
                        t->wipe_tower_id
                    );
                    n.set_tag(new_tag);

                    ColorRGBA clr = ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};
                    auto color_it = Scene::VOLUME_COLORS.find(model_volume->type());
                    if (color_it != Scene::VOLUME_COLORS.end())
                        clr = color_it->second;

                    auto material =
                        Render::Material{}
                            .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
                            .set_uniform("uniform_color", clr)
                            .set_transparent(clr.is_transparent());
                    n.render_component()->replace_material(material);
                }
                return;
            }
        },
        true
    );

    m_volume_materials_dirty = true;
    invoke_bed_visually_changed(project_id);
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
        Scene::build_bed_node(builder, inst, tag, m_device, m_projects[project_id], Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects));

        scn.add_child(builder.build().release());
    }

    update_bed_instances();
    invoke_bed_visually_changed(project_id);
    m_volume_materials_dirty = true;
}

void PlaterScenePresenter::on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances)
{
    remove_beds(project_id, instances);

    bool res = false;
    for (const auto& bed_ref : instances) {
        res |= update_bed_instance_error_state(Domain::SlicingId{ project_id, bed_ref.instance_id }, false);
    }
    if (!res) {
        // when res == true the update has been already performed inside update_bed_instance_error_state()
        update_bed_instances();
        invoke_bed_visually_changed(project_id);
    }

    m_volume_materials_dirty = true;
}

void PlaterScenePresenter::on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances, Biz::Scene::TransformState state)
{
    if (state != Biz::Scene::TransformState::InProgress)
        invoke_bed_visually_changed(project_id);
}

void PlaterScenePresenter::on_virtual_bed_preview_changed(
    Domain::SelectionId project_id,
    const std::optional<Biz::Scene::VirtualBedPreview>& preview
)
{
    auto& scn = project_scene(project_id);

    // Always remove any existing virtual-bed subtree before either rebuilding or hiding.
    Scene::Node* existing = scn.root().query_first(
        [](const Scene::Node* n) {
            const Scene::BedNodeTag* t = n->tag_of_type<Scene::BedNodeTag>();
            return t != nullptr && t->is_virtual && t->type == Scene::BedElementType::Undefined;
        },
        true
    );
    if (existing != nullptr)
        scn.remove_child(existing);

    if (!preview.has_value()) {
        invoke_bed_visually_changed(project_id);
        return;
    }

    const auto& proj = m_workbench.project(project_id);
    const Domain::ConfigContainer* cc = proj.find_config_container(preview->config_container_id);
    if (cc == nullptr)
        return;

    // Transient instance: not owned by the project, lives only for this build call.
    Domain::BedInstance tmp_instance{cc->bed()};
    tmp_instance.transformation = Domain::Transformation(preview->transform);

    Scene::NodeBuilder builder(scn);
    Scene::build_virtual_bed_node(
        builder,
        tmp_instance,
        preview->config_container_id,
        m_device,
        m_projects[project_id],
        Scene::RenderLayerId(PlaterSceneLayer::DocumentObjects)
    );

    auto root = builder.build();
    Scene::Node* root_ptr = root.release();
    scn.add_child(root_ptr);

    update_bed_instances();
    invoke_bed_visually_changed(project_id);
}

static void remove_wipe_tower_node(Scene::Scene& scene, Domain::SlicingId slicing_id) {
    Scene::Node* node{scene.root().query_first([&](const Scene::Node* node){
        auto tag{node->tag_of_type<Scene::SceneNodeTag>()};
        if (tag == nullptr) {
            return false;
        }
        return tag->wipe_tower_id == slicing_id;
    }, true)};
    if (node == nullptr) {
        return;
    }
    scene.remove_child(node);
}

void PlaterScenePresenter::on_wipe_tower_changed(
    Domain::SlicingId slicing_id,
    const Biz::Print::WipeTowerGeometry& wipe_tower
)
{
    auto it{m_projects.find(slicing_id.project_id)};
    if (it == m_projects.end()) {
        return;
    }
    Scene::Scene& project_scene{it->second.scene()};
    remove_wipe_tower_node(project_scene, slicing_id);

    const Domain::Project& project{m_workbench.project(slicing_id.project_id)};
    const Domain::BedInstance* bed_instance{
        project.find_bed_instance_by_id(slicing_id.bed_instance_id)
    };
    if (!bed_instance) {
        return;
    }

    Scene::NodeBuilder builder{project_scene};
    builder.set_debug_name("wipe_tower");
    builder.set_tag(wipe_tower_tag(slicing_id));

    Domain::Transform3d transform{bed_instance->transformation.get_matrix()};
    transform.translate(
        Vec3d{bed_instance->wipe_tower.position.x(), bed_instance->wipe_tower.position.y(), 0.0}
    );
    transform.rotate(Eigen::AngleAxisd{Slic3r::deg2rad(bed_instance->wipe_tower.rotation), Vec3d::UnitZ()});
    builder.set_transform(transform);

    if (!wipe_tower.depths.empty()) {
        build_wipe_tower_node(builder, wipe_tower, slicing_id);
        project_scene.add_child(builder.build().release());
    } else {
        build_unknown_wipe_tower_node(builder, wipe_tower, slicing_id);
        project_scene.add_child(builder.build().release());
    }
    m_volume_materials_dirty = true;
    invoke_bed_visually_changed(slicing_id.project_id);
}

void PlaterScenePresenter::on_wipe_tower_moved(Domain::SlicingId slicing_id)
{
    auto it{m_projects.find(slicing_id.project_id)};
    if (it == m_projects.end()) {
        return;
    }
    Scene::Scene& project_scene{it->second.scene()};
    Scene::Node* node{project_scene.root().query_first([&](const Scene::Node* node){
        auto tag{node->tag_of_type<Scene::SceneNodeTag>()};
        if (tag == nullptr) {
            return false;
        }
        return tag->wipe_tower_id == slicing_id;
    })};
    if (node == nullptr) {
        return;
    }
    const Domain::BedInstance* bed_instance{
        m_workbench.project(slicing_id.project_id)
            .find_bed_instance_by_id(slicing_id.bed_instance_id)
    };
    if (bed_instance == nullptr) {
        return;
    }

    const Domain::ModelWipeTower wipe_tower{bed_instance->wipe_tower};

    Domain::Transform3d transform{bed_instance->transformation.get_matrix()};
    transform.translate(Vec3d{wipe_tower.position.x(), wipe_tower.position.y(), 0.0});
    transform.rotate(Eigen::AngleAxisd{Slic3r::deg2rad(wipe_tower.rotation), Vec3d::UnitZ()});
    node->set_local_transform(transform);
}

void PlaterScenePresenter::on_wipe_tower_removed(Domain::SlicingId slicing_id)
{
    auto it{m_projects.find(slicing_id.project_id)};
    if (it == m_projects.end()) {
        return;
    }
    Scene::Scene& project_scene{it->second.scene()};
    remove_wipe_tower_node(project_scene, slicing_id);
    invoke_bed_visually_changed(slicing_id.project_id);
}

void PlaterScenePresenter::on_layer_begin(Render::CommandBuffer& cmd_buf, Scene::RenderLayerId layer_idx)
{
    cmd_buf.set_depth_write_enabled(true);
    if (layer_idx == Scene::RenderLayerId(PlaterSceneLayer::GizmoHandles)) {
        // clear depth buffer so all gizmo handles are rendered over document objects
        cmd_buf.clear_buffers(false, true);
        cmd_buf.set_depth_test_enabled(true);
    }
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
}

} // namespace Slic3r::App::Plater
