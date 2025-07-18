///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/MeasureGizmo.hpp"
#include "Slic3r/App/Plater/MeasureDialog.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/AabbRaycastNodeComponent.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/Math.hpp"

#include <magic_enum/magic_enum_flags.hpp>
// DEBUG ONLY
#include <imgui/imgui.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Plater::Measure;
using namespace magic_enum::bitwise_operators;

namespace Slic3r::App::Plater {

static constexpr double SPHERE_RADIUS                    = 0.5;
static constexpr double SPHERE_RESOLUTION_ANGLE          = Slic3r::deg2rad(360.0 / 64.0);
static constexpr double CYLINDER_RADIUS                  = 0.5;
static constexpr double CYLINDER_RESOLUTION_ANGLE        = Slic3r::deg2rad(360.0 / 64.0);
static constexpr double TORUS_RADIUS                     = 0.5;
static constexpr double TORUS_MAIN_RESOLUTION_ANGLE      = Slic3r::deg2rad(360.0 / 64.0);
static constexpr double TORUS_SECONDARY_RESOLUTION_ANGLE = Slic3r::deg2rad(360.0 / 16.0);

static const Domain::ColorRGBA FIRST_FEATURE_COLOR          = {0.25f, 0.75f, 0.75f, 1.0f};
static const Domain::ColorRGBA SECOND_FEATURE_COLOR         = {0.75f, 0.25f, 0.75f, 1.0f};
static const Domain::ColorRGBA FIRST_FEATURE_HOVERED_COLOR  = {0.35f, 0.85f, 0.85f, 1.0f};
static const Domain::ColorRGBA SECOND_FEATURE_HOVERED_COLOR = {0.85f, 0.35f, 0.85f, 1.0f};

static const std::string CURRENT_FEATURE_NAME = "current feature";
static const std::string FIRST_FEATURE_NAME   = "first selected feature";
static const std::string SECOND_FEATURE_NAME  = "second selected feature";

static constexpr float TRIANGLE_BASE   = 15.0f;
static constexpr float TRIANGLE_HEIGHT = TRIANGLE_BASE * 1.618033f;

MeasureGizmo::MeasureGizmo(
    Render::Device& device,
    Biz::ProjectInteractor& project_interactor,
    PlaterScenePresenter& scene_presenter
) :
    m_device(device),
    m_project_interactor(project_interactor),
    m_scene_interactor(project_interactor.scene_interactor()),
    m_projects(project_interactor),
    m_scene_presenter(scene_presenter)
{
    m_dialog = std::make_unique<MeasureDialog>();
}

void MeasureGizmo::on_activated()
{
    m_current_project = &m_projects.project(m_project_interactor.selected_project_id());
    // DEBUG ONLY
    m_current_project->id = m_project_interactor.selected_project_id();

    auto& scene = m_scene_presenter.scene();

    Scene::NodeBuilder builder{scene};
    builder.set_debug_name("measure gizmo main");
    builder.set_tag(MeasureGizmoNodeTag{});

    auto main_node = builder.build();
    m_main_node    = main_node.get();
    scene.add_child(main_node.release(), &scene.root());

    on_scene_selection_changed(
        m_project_interactor.selected_project_id(),
        m_scene_interactor.object_selection()
    );
}

void MeasureGizmo::on_deactivated()
{
    reset();
    m_scene_presenter.scene().remove_child(m_main_node);
    m_main_node         = nullptr;
    m_dimensioning_node = nullptr;
}

Scene::ToolType MeasureGizmo::type() const
{
    return Scene::ToolType::MeasureGizmo;
}

Yoga::Dialog* MeasureGizmo::unload_ui_dialog()
{
    return m_dialog.get();
}

std::optional<FeatureItem> MeasureGizmo::detect_current_feature()
{
    std::optional<FeatureItem> ret;
    if (m_feature_detection_data.has_value()) {
        const Scene::AabbRaycastNodeComponent*
            raycast_component = dynamic_cast<const Scene::AabbRaycastNodeComponent*>(
                m_feature_detection_data->node->raycast_component()
            );
        DEBUG_ASSERT(raycast_component != nullptr);
        AABBMesh::hit_result hit_result = raycast_component->hit_result(
            m_feature_detection_data->hovered_volume->world_trafo.matrix(),
            m_feature_detection_data->pick_ray
        );
        if (hit_result.is_hit()) {
            Domain::Vec3d hit_position = m_feature_detection_data->pick_ray.origin
                + m_feature_detection_data->pick_ray.direction * m_feature_detection_data->hit_coord;
            int hit_face_id = hit_result.face()
                + m_feature_detection_data->hovered_volume->face_offset;
            std::optional<SurfaceFeature> feature = m_feature_detection_data->hovered_instance
                                                        ->measuring->feature(hit_face_id, hit_position);
            if (feature.has_value()) {
                if (m_selection_mode == SelectionMode::Point
                    && feature->type() != SurfaceFeatureType::Point)
                    // TODO: replace hit_position with call to position_on_feature() from old PrusaSlicer
                    ret = {
                        m_feature_detection_data->hovered_instance->ref,
                        SurfaceFeature(hit_position),
                        feature
                    };
                else
                    ret = {m_feature_detection_data->hovered_instance->ref, *feature};
            }
        }
    }
    return ret;
}

Scene::GizmoActivationState MeasureGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    if (m_current_project->scene_selection_cache.volumes.empty())
        // selection is empty
        return Scene::GizmoActivationState::Inactive;

    bool is_left_button = (ctx.mouse_event().button() & Platform::MouseButton::Left)
        == Platform::MouseButton::Left;
    auto event_type = ctx.mouse_event().type();
    if (event_type == Platform::MouseEvent::Type::ButtonDown && is_left_button)
        m_mouse_left_down = true;

    // remove highlights
    Scene::visit(*m_main_node, [&](Scene::Node& node) {
        if (node.tag_of_type<MeasureGizmoNodeTag>() != nullptr)
            node.remove_material_override();
    });

    auto& feature_cache = m_current_project->feature_cache;

    Scene::Node* hovered_feature_node = ctx.pick_result_node_with_tag_of_type<MeasureGizmoNodeTag>();
    Scene::Node* hovered_scene_node = ctx.pick_result_node_with_tag_of_type<SceneNodeTag>();

    update_feature_detection_data(hovered_scene_node, ctx);

    if (hovered_scene_node != nullptr && !m_feature_detection_data.has_value()) {
        // hovering an unselected volume
        feature_cache.current.reset();
        update_current_feature_on_scene();
        return Scene::GizmoActivationState::Inactive;
    }

    if (hovered_feature_node != nullptr) {
        highlight_node(*hovered_feature_node);

        // hovering a feature
        if (m_mouse_left_down) {
            const MeasureGizmoNodeTag& tag = *hovered_feature_node->tag_of_type<MeasureGizmoNodeTag>();
            switch (tag.type) {
            case MeasureGizmoElementType::CurrentFeature: {
                handle_left_click_on_current_feature(*hovered_feature_node);
                break;
            }
            case MeasureGizmoElementType::FirstSelectedFeature: {
                handle_left_click_on_first_selected_feature(*hovered_feature_node);
                break;
            }
            case MeasureGizmoElementType::SecondSelectedFeature: {
                handle_left_click_on_second_selected_feature(*hovered_feature_node);
                break;
            }
            }
        } else {
            feature_cache.current = detect_current_feature();
            update_current_feature_on_scene();
        }
    } else {
        if (hovered_scene_node == nullptr) {
            // nothing is hovered
            if (feature_cache.current.has_value()) {
                feature_cache.current.reset();
                update_current_feature_on_scene();
            }
            return Scene::GizmoActivationState::Inactive;
        } else {
            // hovering a scene volume
            feature_cache.current = detect_current_feature();
            update_current_feature_on_scene();
        }
    }

    return Scene::GizmoActivationState::Inactive;
}

void MeasureGizmo::on_transient_mouse(Scene::GizmoEventContext& ctx)
{
    // m_main_node != nullptr means that the MeasureGizmo is active
    if (m_main_node != nullptr) {
        bool is_left_button = (ctx.mouse_event().button() & Platform::MouseButton::Left)
            == Platform::MouseButton::Left;
        auto event_type = ctx.mouse_event().type();
        if (m_mouse_left_down
            && ((is_left_button && event_type == Platform::MouseEvent::Type::ButtonUp)
                || event_type == Platform::MouseEvent::Type::Leave))
            m_mouse_left_down = false;
    }
}

void MeasureGizmo::on_keyboard(Scene::GizmoKeyEventContext& ctx)
{
    const Platform::KeyboardEvent& evt = ctx.keyboard_event();
    if (m_main_node == nullptr || evt.repeat())
        return;

    Platform::KeyCode code = evt.code();
    if (code == Platform::KeyCode::RShift || code == Platform::KeyCode::LShift) {
        m_selection_mode = (evt.type() == Platform::KeyboardEvent::Type::KeyDown) ?
            SelectionMode::Point :
            SelectionMode::Feature;

        m_current_project->feature_cache.current = detect_current_feature();
        update_current_feature_on_scene();
    }
}

void MeasureGizmo::on_project_activated(size_t new_project_id)
{
    m_current_project = &m_projects.project(new_project_id);
    // DEBUG ONLY
    m_current_project->id = new_project_id;

    m_selection_mode = SelectionMode::Feature;
    m_dialog->show_measure(!m_current_project->scene_selection_cache.volumes.empty());
}

void MeasureGizmo::render_scene(Render::CommandBuffer& cmd_buffer)
{
    if (!m_mouse_left_down)
        update_scene_selection_cache_measuring_geometry();

    render_dimensioning();
}

void MeasureGizmo::register_commands(Platform::CommandRegistry& registry)
{
    registry
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "measure-gizmo-unselect-feature",
                [this]() {
                    if (!m_current_project->feature_cache.selected[0].has_value())
                        return;

                    if (m_current_project->feature_cache.selected[1].has_value()) {
                        // remove second selected feature
                        m_current_project->feature_cache.selected[1].reset();
                        remove_feature_from_scene(MeasureGizmoElementType::SecondSelectedFeature);
                    } else {
                        // remove first selected feature
                        m_current_project->feature_cache.selected[0].reset();
                        remove_feature_from_scene(MeasureGizmoElementType::FirstSelectedFeature);
                    }

                    update_measurement_result();
                    update_ui_dialog();
                },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Escape}
            )
        )
        .register_command(
            std::make_unique<Platform::FuncCommand>(
                "measure-gizmo-unselect-all_features",
                [this]() {
                    m_current_project->feature_cache.selected[1].reset();
                    remove_feature_from_scene(MeasureGizmoElementType::SecondSelectedFeature);
                    m_current_project->feature_cache.selected[0].reset();
                    remove_feature_from_scene(MeasureGizmoElementType::FirstSelectedFeature);

                    update_measurement_result();
                    update_ui_dialog();
                },
                nullptr,
                Platform::KeyboardShortcut{0, Platform::KeyCode::Delete}
            )
        );
}

static Domain::ElementRefs extract_volume_ids_from_selection(
    const Biz::Scene::ObjectSelection& selection,
    const Domain::Project& project
)
{
    Domain::ElementRefs ret;
    for (const auto& ref : selection.elements) {
        if (ref.volume_id == 0) {
            const Domain::ModelObject* object = project.find_object_by_id(ref.object_id);
            for (const auto volume : object->volumes) {
                ret.push_back({ref.object_id, ref.instance_id, volume->id().id});
            }
        } else
            ret.push_back(ref);
    }
    return ret;
}

// returns a pair of removed and added volumes
static std::pair<Domain::ElementRefs, Domain::ElementRefs> compare_volume_ids(
    Domain::ElementRefs old_ids,
    Domain::ElementRefs new_ids
)
{
    std::sort(old_ids.begin(), old_ids.end());
    std::sort(new_ids.begin(), new_ids.end());

    std::pair<Domain::ElementRefs, Domain::ElementRefs> ret;
    auto& [removed, added] = ret;
    std::set_difference(
        old_ids.begin(),
        old_ids.end(),
        new_ids.begin(),
        new_ids.end(),
        std::back_inserter(removed)
    );
    std::set_difference(
        new_ids.begin(),
        new_ids.end(),
        old_ids.begin(),
        old_ids.end(),
        std::back_inserter(added)
    );
    return ret;
}

void MeasureGizmo::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    // the gizmo is not active
    if (m_main_node == nullptr)
        return;

    if (selection.empty()) {
        reset();
        return;
    }

    m_dialog->show_measure(true);

    Domain::ElementRefs& volumes_ids = m_current_project->scene_selection_cache.volume_ids;
    Domain::ElementRefs volumes_from_selection = extract_volume_ids_from_selection(
        selection,
        m_project_interactor.selected_project()
    );
    if (volumes_ids != volumes_from_selection) {
        auto [removed, added] = compare_volume_ids(volumes_ids, volumes_from_selection);
        update_scene_selection_cache_state(removed, added);
        volumes_ids = volumes_from_selection;
        Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
    }
}

void MeasureGizmo::on_scene_selection_transformed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    // the gizmo is not active
    if (m_main_node == nullptr)
        return;

    m_current_project->feature_cache.reset();
    update_measurement_result();
    update_ui_dialog();
    clear_scene();

    // detect instances to update
    auto& instances = m_current_project->scene_selection_cache.instances;
    for (const auto& elem : selection.elements) {
        auto inst_it = std::find_if(
            instances.begin(),
            instances.end(),
            [&elem](const InstanceCacheItem& item) {
                return item.ref.object_id == elem.object_id
                    && item.ref.instance_id == elem.instance_id;
            }
        );
        DEBUG_ASSERT(inst_it != instances.end());
        inst_it->modified = true;
    }
    Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
}

// DEBUG ONLY
void MeasureGizmo::render_imgui()
{
    const auto& scene_selection_cache = m_current_project->scene_selection_cache;
    const auto& feature_cache         = m_current_project->feature_cache;

    if (ImGui::Begin("Measure gizmo debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Project", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Project ID");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", m_current_project->id);
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("SelectionMode", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Selection mode");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", (m_selection_mode == SelectionMode::Feature) ? "Feature" : "Point");
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("MouseLeftDown", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Mouse left down");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", m_mouse_left_down ? "True" : "False");
            ImGui::EndTable();
        }

        ImGui::Text("Scene selection cache");
        if (scene_selection_cache.volume_ids.empty())
            ImGui::Text("EMPTY");
        else {
            ImGui::Text("Selected volumes ids");
            if (ImGui::BeginTable("SelectedVolumesIds", 2, ImGuiTableFlags_Borders)) {
                for (size_t i = 0; i < scene_selection_cache.volume_ids.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Volume %d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(
                        "o:%d, i:%d, v:%d",
                        scene_selection_cache.volume_ids[i].object_id,
                        scene_selection_cache.volume_ids[i].instance_id,
                        scene_selection_cache.volume_ids[i].volume_id
                    );
                }
                ImGui::EndTable();
            }
            ImGui::Text("Volume cache");
            if (ImGui::BeginTable("VolumeCache", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Volume");
                ImGui::TableSetupColumn("Offset");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < scene_selection_cache.volumes.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(
                        "o:%d, i:%d, v:%d",
                        scene_selection_cache.volumes[i].ref.object_id,
                        scene_selection_cache.volumes[i].ref.instance_id,
                        scene_selection_cache.volumes[i].ref.volume_id
                    );
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", scene_selection_cache.volumes[i].face_offset);
                }
                ImGui::EndTable();
            }
            ImGui::Text("Instance cache");
            if (ImGui::BeginTable("InstanceCache", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Instance");
                ImGui::TableSetupColumn("Modified");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < scene_selection_cache.instances.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(
                        "o:%d, i:%d",
                        scene_selection_cache.instances[i].ref.object_id,
                        scene_selection_cache.instances[i].ref.instance_id
                    );
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", scene_selection_cache.instances[i].modified ? "true" : "false");
                }
                ImGui::EndTable();
            }
        }
        ImGui::Separator();
        ImGui::Text("Current feature");
        if (feature_cache.current.has_value()) {
            std::string text;
            switch (feature_cache.current->feature.type()) {
            default:
            case SurfaceFeatureType::Undefined: {
                text = "Undefined";
                break;
            }
            case SurfaceFeatureType::Point: {
                if (feature_cache.current->parent.has_value()) {
                    text = "Point on ";
                    switch (feature_cache.current->parent->type()) {
                    default:
                    case SurfaceFeatureType::Undefined: {
                        text += "undefined";
                        break;
                    }
                    case SurfaceFeatureType::Point: {
                        text += "point";
                        break;
                    }
                    case SurfaceFeatureType::Edge: {
                        text += "edge";
                        break;
                    }
                    case SurfaceFeatureType::Circle: {
                        text += "circle";
                        break;
                    }
                    case SurfaceFeatureType::Plane: {
                        text += "plane";
                        break;
                    }
                    }
                } else
                    text = "Vertex";
                break;
            }
            case SurfaceFeatureType::Edge: {
                text = "Edge";
                break;
            }
            case SurfaceFeatureType::Circle: {
                text = "Circle";
                break;
            }
            case SurfaceFeatureType::Plane: {
                text = "Plane";
                break;
            }
            }
            ImGui::Text("%s", text.c_str());
        } else
            ImGui::Text("EMPTY");
    }
    ImGui::End();
}

void MeasureGizmo::reset()
{
    m_current_project->scene_selection_cache.reset();
    m_current_project->feature_cache.reset();
    update_measurement_result();
    update_ui_dialog();
    clear_scene();
    m_dialog->show_measure(false);
}

void MeasureGizmo::update_scene_selection_cache_state(
    const Domain::ElementRefs& removed_volumes,
    const Domain::ElementRefs& added_volumes
)
{
    auto& volumes   = m_current_project->scene_selection_cache.volumes;
    auto& instances = m_current_project->scene_selection_cache.instances;

    const Domain::Project& project = m_project_interactor.selected_project();

    // update volume cache and detect instances to update
    for (const auto& v : removed_volumes) {
        auto vol_it = std::find_if(volumes.begin(), volumes.end(), [&v](const VolumeCacheItem& item) {
            return item.ref == v;
        });
        DEBUG_ASSERT(vol_it != volumes.end());
        volumes.erase(vol_it);

        auto inst_it = std::find_if(
            instances.begin(),
            instances.end(),
            [&v](const InstanceCacheItem& item) {
                return item.ref.object_id == v.object_id && item.ref.instance_id == v.instance_id;
            }
        );
        DEBUG_ASSERT(inst_it != instances.end());
        inst_it->modified = true;
    }

    for (const auto& v : added_volumes) {
        const Domain::ModelObject* object = project.find_object_by_id(v.object_id);
        const Domain::ModelInstance* instance = project.find_instance_by_id(v.object_id, v.instance_id);
        if (v.has_volume()) {
            const Domain::ModelVolume* volume = project.find_volume_by_id(v.object_id, v.volume_id);
            VolumeCacheItem item;
            item.ref         = v;
            item.mesh        = &volume->mesh();
            item.world_trafo = instance->get_matrix() * volume->get_matrix();
            volumes.emplace_back(item);
        } else {
            for (const auto volume : object->volumes) {
                VolumeCacheItem item;
                item.ref         = {v.object_id, v.instance_id, volume->id().id};
                item.mesh        = &volume->mesh();
                item.world_trafo = instance->get_matrix() * volume->get_matrix();
                volumes.emplace_back(item);
            }
        }

        auto inst_it = std::find_if(
            instances.begin(),
            instances.end(),
            [&v](const InstanceCacheItem& item) {
                return item.ref.object_id == v.object_id && item.ref.instance_id == v.instance_id;
            }
        );
        if (inst_it == instances.end()) {
            InstanceCacheItem item;
            item.ref = {v.object_id, v.instance_id, 0};
            instances.push_back(std::move(item));
            inst_it = std::prev(instances.end());
        }
        inst_it->modified = true;
    }
}

void MeasureGizmo::update_scene_selection_cache_measuring_geometry()
{
    const Domain::Project& project = m_project_interactor.selected_project();
    auto& volumes                  = m_current_project->scene_selection_cache.volumes;
    auto& instances                = m_current_project->scene_selection_cache.instances;

    for (size_t i = 0; i < instances.size(); ++i) {
        InstanceCacheItem& inst_item = instances[i];
        if (inst_item.modified) {
            // removes instance render geometry from the scene
            auto& scene = m_scene_presenter.scene();
            if (inst_item.node != nullptr)
                scene.remove_child(inst_item.node);

            // check if all the volumes of the instance have been removed
            size_t vol_count = std::count_if(
                volumes.begin(),
                volumes.end(),
                [&inst_item](const VolumeCacheItem& vol_item) {
                    return vol_item.ref.object_id == inst_item.ref.object_id
                        && vol_item.ref.instance_id == inst_item.ref.instance_id;
                }
            );
            // and if so, remove the instance too
            if (vol_count == 0) {
                instances.erase(instances.begin() + i);
                --i;
            } else {
                // generate new measuring object for the instance by composing the volume meshes
                const Domain::ModelInstance* instance = project.find_instance_by_id(
                    inst_item.ref.object_id,
                    inst_item.ref.instance_id
                );
                Domain::Transform3d inst_matrix = instance->get_matrix();
                Domain::TriangleMesh composite_mesh;
                for (auto& vol_item : volumes) {
                    if (vol_item.ref.object_id == inst_item.ref.object_id
                        && vol_item.ref.instance_id == inst_item.ref.instance_id)
                    {
                        vol_item.world_trafo = inst_matrix
                            * project
                                  .find_volume_by_id(vol_item.ref.object_id, vol_item.ref.volume_id)
                                  ->get_matrix();
                        vol_item.face_offset             = int(composite_mesh.facets_count());
                        Domain::TriangleMesh volume_mesh = *vol_item.mesh;
                        volume_mesh.transform(vol_item.world_trafo);
                        composite_mesh.merge(volume_mesh);
                    }
                }
                inst_item.measuring.reset(new Measuring(composite_mesh.its));
                inst_item.modified = false;
            }
        }
    }
}

void MeasureGizmo::update_feature_detection_data(
    const Scene::Node* scene_node,
    const Scene::GizmoEventContext& ctx
)
{
    if (scene_node == nullptr) {
        // nothing is hovered
        m_feature_detection_data.reset();
        return;
    }

    const SceneSelectionCache& cache = m_current_project->scene_selection_cache;
    const SceneNodeTag* tag          = scene_node->tag_of_type<SceneNodeTag>();
    DEBUG_ASSERT(tag != nullptr);

    auto vol_it = std::find_if(
        cache.volumes.begin(),
        cache.volumes.end(),
        [tag](const VolumeCacheItem& item) {
            return item.ref.object_id == tag->object_id
                && item.ref.instance_id == tag->instance_id
                && item.ref.volume_id == tag->volume_id;
        }
    );

    if (vol_it == cache.volumes.end()) {
        // hovering an unselected volume
        m_feature_detection_data.reset();
        return;
    }

    auto inst_it = std::find_if(
        cache.instances.begin(),
        cache.instances.end(),
        [&vol_it](const InstanceCacheItem& item) {
            return item.ref.object_id == vol_it->ref.object_id
                && item.ref.instance_id == vol_it->ref.instance_id;
        }
    );
    DEBUG_ASSERT(inst_it != cache.instances.end());

    m_feature_detection_data = FeatureDetectionData{
        &(*vol_it),
        &(*inst_it),
        scene_node,
        ctx.pick_ray(),
        ctx.pick_results().front().t
    };
}

void MeasureGizmo::update_current_feature_on_scene()
{
    remove_feature_from_scene(MeasureGizmoElementType::CurrentFeature);
    if (m_current_project->feature_cache.current.has_value()) {
        const FeatureItem& current_feature = *m_current_project->feature_cache.current;
        const auto& instances              = m_current_project->scene_selection_cache.instances;
        auto inst_it                       = std::find_if(
            instances.begin(),
            instances.end(),
            [&current_feature](const InstanceCacheItem& item) {
                return item.ref.object_id == current_feature.ref.object_id
                    && item.ref.instance_id == current_feature.ref.instance_id;
            }
        );
        DEBUG_ASSERT(inst_it != instances.end());

        add_feature_to_scene(
            current_feature,
            MeasureGizmoElementType::CurrentFeature,
            CURRENT_FEATURE_NAME,
            m_current_project->feature_cache.selected[0].has_value() ? SECOND_FEATURE_COLOR :
                                                                       FIRST_FEATURE_COLOR,
            *inst_it->measuring
        );
    }
}

void MeasureGizmo::update_measurement_result()
{
    m_measurement_result = MeasurementResult();
    auto& feature_cache  = m_current_project->feature_cache;
    if (feature_cache.selected[1].has_value()) {
        const auto& instances = m_current_project->scene_selection_cache.instances;

        //
        // measuring is needed only in case of edge-plane or circle-plane measurements
        // it must come from the feature containing the plane
        //
        SurfaceFeatureType f0 = feature_cache.selected[0]->feature.type();
        SurfaceFeatureType f1 = feature_cache.selected[1]->feature.type();
        Measuring* measuring  = nullptr;
        int id                = -1;
        if (f0 == SurfaceFeatureType::Plane
            && (f1 == SurfaceFeatureType::Edge || f1 == SurfaceFeatureType::Circle))
            id = 0;
        else if (f1 == SurfaceFeatureType::Plane
                 && (f0 == SurfaceFeatureType::Edge || f0 == SurfaceFeatureType::Circle))
            id = 1;
        if (id != -1) {
            auto inst_it = std::find_if(
                instances.begin(),
                instances.end(),
                [&feature_cache, id](const InstanceCacheItem& item) {
                    return item.ref.object_id == feature_cache.selected[id]->ref.object_id
                        && item.ref.instance_id == feature_cache.selected[id]->ref.instance_id;
                }
            );
            DEBUG_ASSERT(inst_it != instances.end());
            measuring = inst_it->measuring.get();
        }
        m_measurement_result = measurement(
            feature_cache.selected[0]->feature,
            feature_cache.selected[1]->feature,
            measuring
        );
    }
}

void MeasureGizmo::update_ui_dialog()
{
    if (m_current_project->feature_cache.selected[0].has_value())
        m_dialog->spot1().set_from(*m_current_project->feature_cache.selected[0]);
    else
        m_dialog->spot1().reset();

    if (m_current_project->feature_cache.selected[1].has_value())
        m_dialog->spot2().set_from(*m_current_project->feature_cache.selected[1]);
    else
        m_dialog->spot2().reset();

    m_dialog->set_measure(m_measurement_result);
}

void MeasureGizmo::highlight_node(Scene::Node& node)
{
    const MeasureGizmoNodeTag* tag = node.tag_of_type<MeasureGizmoNodeTag>();
    if (tag != nullptr) {
        Domain::ColorRGBA color;
        switch (tag->type) {
        case MeasureGizmoElementType::CurrentFeature: {
            color = m_current_project->feature_cache.selected[0].has_value() ?
                SECOND_FEATURE_HOVERED_COLOR :
                FIRST_FEATURE_HOVERED_COLOR;
            break;
        }
        case MeasureGizmoElementType::FirstSelectedFeature: {
            color = FIRST_FEATURE_HOVERED_COLOR;
            break;
        }
        case MeasureGizmoElementType::SecondSelectedFeature: {
            color = SECOND_FEATURE_HOVERED_COLOR;
            break;
        }
        default: {
            return;
        }
        }
        Render::Material material = node.render_component()->material();
        material.set_uniform("uniform_color", color);
        node.set_material_override(material);
    }
}

void MeasureGizmo::clear_scene()
{
    m_scene_presenter.scene().remove_children(
        [](const Scene::Node* node) {
            const MeasureGizmoNodeTag* t = node->tag_of_type<MeasureGizmoNodeTag>();
            return t != nullptr;
        },
        m_main_node
    );

    m_dimensioning_node = nullptr;

    m_geometry_manager.release_all();
    m_triangle_mesh_manager.release_all();
}

void MeasureGizmo::handle_left_click_on_current_feature(Scene::Node& feature_node)
{
    auto& feature_cache = m_current_project->feature_cache;
    if (!feature_cache.selected[0].has_value()) {
        // add first selected feature
        feature_cache.selected[0] = feature_cache.current;
        feature_node.set_debug_name(FIRST_FEATURE_NAME);
        feature_node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::FirstSelectedFeature});
        feature_cache.current.reset();
    } else {
        // add or replace second selected feature
        if (feature_cache.selected[1].has_value())
            remove_feature_from_scene(MeasureGizmoElementType::SecondSelectedFeature);

        feature_cache.selected[1] = feature_cache.current;
        feature_node.set_debug_name(SECOND_FEATURE_NAME);
        feature_node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::SecondSelectedFeature});
        feature_cache.current.reset();
    }

    update_measurement_result();
    update_ui_dialog();
}

void MeasureGizmo::handle_left_click_on_first_selected_feature(Scene::Node& feature_node)
{
    auto& feature_cache = m_current_project->feature_cache;
    if (!feature_cache.selected[1].has_value()) {
        // remove first selected feature
        feature_cache.current = feature_cache.selected[0];
        feature_node.set_debug_name(CURRENT_FEATURE_NAME);
        feature_node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::CurrentFeature});
        feature_cache.selected[0].reset();
    } else {
        // replace first selected feature with second selected feature
        feature_cache.current = feature_cache.selected[0];
        feature_node.set_debug_name(CURRENT_FEATURE_NAME);
        feature_node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::CurrentFeature});
        Render::Material material = feature_node.render_component()->material();
        material.set_uniform("uniform_color", SECOND_FEATURE_COLOR);
        feature_node.render_component()->replace_material(material);

        feature_cache.selected[0] = feature_cache.selected[1];
        feature_cache.selected[1].reset();
        Scene::visit(*m_main_node, [&](Scene::Node& node) {
            const MeasureGizmoNodeTag& tag = *node.tag_of_type<MeasureGizmoNodeTag>();
            if (tag.type == MeasureGizmoElementType::SecondSelectedFeature) {
                node.set_debug_name(FIRST_FEATURE_NAME);
                node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::FirstSelectedFeature});
                Render::Material material = node.render_component()->material();
                material.set_uniform("uniform_color", FIRST_FEATURE_COLOR);
                node.render_component()->replace_material(material);
            }
        });
    }

    update_measurement_result();
    update_ui_dialog();
}

void MeasureGizmo::handle_left_click_on_second_selected_feature(Scene::Node& feature_node)
{
    auto& feature_cache = m_current_project->feature_cache;

    // remove second selected feature
    feature_cache.current = feature_cache.selected[1];
    feature_node.set_debug_name(CURRENT_FEATURE_NAME);
    feature_node.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::CurrentFeature});
    feature_cache.selected[1].reset();

    update_measurement_result();
    update_ui_dialog();
}

void MeasureGizmo::add_feature_to_scene(
    const FeatureItem& feature,
    MeasureGizmoElementType type,
    const std::string& debug_name,
    const Domain::ColorRGBA& color,
    const Measuring& measuring
)
{
    auto& scene = m_scene_presenter.scene();
    Scene::NodeBuilder builder{scene};
    if (!debug_name.empty())
        builder.set_debug_name(debug_name);
    builder.set_tag(MeasureGizmoNodeTag{type});
    switch (feature.feature.type()) {
    case SurfaceFeatureType::Point: {
        build_point_feature(builder, feature, color);
        break;
    }
    case SurfaceFeatureType::Edge: {
        build_edge_feature(builder, feature, color);
        break;
    }
    case SurfaceFeatureType::Circle: {
        build_circle_feature(builder, feature, color);
        break;
    }
    case SurfaceFeatureType::Plane: {
        build_plane_feature(builder, feature, measuring, color);
        break;
    }
    }

    scene.add_child(builder.build().release(), m_main_node);
}

void MeasureGizmo::remove_feature_from_scene(MeasureGizmoElementType type)
{
    m_scene_presenter.scene().remove_children(
        [type](const Scene::Node* node) {
            const MeasureGizmoNodeTag* tag = node->tag_of_type<MeasureGizmoNodeTag>();
            return tag != nullptr && tag->type == type;
        },
        m_main_node
    );
}

void MeasureGizmo::build_point_feature(
    Scene::NodeBuilder& builder,
    const FeatureItem& feature,
    const Domain::ColorRGBA& color
)
{
    Domain::Vec3d point = feature.feature.point();

    std::string id = "point_feature";

    auto trimesh = m_triangle_mesh_manager.get_or_create(id, [this]() {
        Domain::TriangleMesh mesh = Biz::Algorithms::TriangleMesh::make_sphere(
            SPHERE_RADIUS,
            SPHERE_RESOLUTION_ANGLE
        );
        return std::make_unique<Scene::TriangleMesh>(std::move(mesh.its));
    });
    auto geom    = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });

    Render::Material material = Render::Material{}
                                    .set_shader(
                                        m_device.context().shader_manager().shader("gouraud_light")
                                    )
                                    .set_uniform("uniform_color", color);

    Domain::Transform3d xform = Domain::Transform3d::Identity();
    xform.translate(point);

    builder.set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
        .set_aabb(trimesh->aabb_mesh())
        .set_transform(xform);
}

void MeasureGizmo::build_edge_feature(
    Scene::NodeBuilder& builder,
    const FeatureItem& feature,
    const Domain::ColorRGBA& color
)
{
    auto [from, to] = feature.feature.edge();
    double length   = (to - from).norm();

    std::string id = fmt::format("edge_feature_{}", length);

    auto trimesh = m_triangle_mesh_manager.get_or_create(id, [&]() {
        Domain::TriangleMesh mesh = Biz::Algorithms::TriangleMesh::make_cylinder(
            CYLINDER_RADIUS,
            length,
            CYLINDER_RESOLUTION_ANGLE
        );
        return std::make_unique<Scene::TriangleMesh>(std::move(mesh.its));
    });
    auto geom    = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });

    Render::Material material = Render::Material{}
                                    .set_shader(
                                        m_device.context().shader_manager().shader("gouraud_light")
                                    )
                                    .set_uniform("uniform_color", color);

    Domain::Vec3d unit_dir = (to - from).normalized();
    auto q                 = Eigen::Quaterniond{}.FromTwoVectors(Domain::Vec3d::UnitZ(), unit_dir);
    Domain::Transform3d xform        = Domain::Transform3d::Identity();
    xform.matrix().block<3, 3>(0, 0) = q.toRotationMatrix();
    xform.matrix().block<3, 1>(0, 3) = from;

    builder.set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
        .set_aabb(trimesh->aabb_mesh())
        .set_transform(xform);
}

void MeasureGizmo::build_circle_feature(
    Scene::NodeBuilder& builder,
    const FeatureItem& feature,
    const Domain::ColorRGBA& color
)
{
    auto [center, radius, normal] = feature.feature.circle();

    std::string id = fmt::format("circle_feature_{}", radius);

    auto trimesh = m_triangle_mesh_manager.get_or_create(id, [&]() {
        Domain::TriangleMesh mesh = Biz::Algorithms::TriangleMesh::make_torus(
            radius,
            TORUS_RADIUS,
            TORUS_MAIN_RESOLUTION_ANGLE,
            TORUS_SECONDARY_RESOLUTION_ANGLE
        );
        return std::make_unique<Scene::TriangleMesh>(std::move(mesh.its));
    });
    auto geom    = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });

    Render::Material material = Render::Material{}
                                    .set_shader(
                                        m_device.context().shader_manager().shader("gouraud_light")
                                    )
                                    .set_uniform("uniform_color", color);

    auto q                    = Eigen::Quaterniond{}.FromTwoVectors(Domain::Vec3d::UnitZ(), normal);
    Domain::Transform3d xform = Domain::Transform3d::Identity();
    xform.matrix().block<3, 3>(0, 0) = q.toRotationMatrix();
    xform.matrix().block<3, 1>(0, 3) = center;

    builder.set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
        .set_aabb(trimesh->aabb_mesh())
        .set_transform(xform);
}

void MeasureGizmo::build_plane_feature(
    Scene::NodeBuilder& builder,
    const FeatureItem& feature,
    const Measuring& measuring,
    const Domain::ColorRGBA& color
)
{
    auto [plane_id, normal, point]          = feature.feature.plane();
    const indexed_triangle_set& its         = measuring.its();
    const std::vector<int>& plane_triangles = measuring.plane_triangle_indices(plane_id);

    // small offset to avoid z-fighting
    Domain::Vec3f offset = 0.01f * normal.cast<float>();

    indexed_triangle_set mesh_its;
    mesh_its.vertices.reserve(3 * plane_triangles.size());
    mesh_its.indices.reserve(3 * plane_triangles.size());
    for (size_t i = 0; i < plane_triangles.size(); ++i) {
        const Domain::Index3& src_triangle = its.indices[plane_triangles[i]];
        size_t base                        = i * 3;
        mesh_its.vertices.emplace_back(offset + its.vertices[src_triangle[0]]);
        mesh_its.vertices.emplace_back(offset + its.vertices[src_triangle[1]]);
        mesh_its.vertices.emplace_back(offset + its.vertices[src_triangle[2]]);
        Domain::Index3 dst_triangle = {int(base + 0), int(base + 1), int(base + 2)};
        mesh_its.indices.emplace_back(dst_triangle);
    }

    std::string id = fmt::format(
        "plane_feature_{}_{}_{}",
        feature.ref.object_id,
        feature.ref.instance_id,
        plane_id
    );

    auto trimesh = m_triangle_mesh_manager.get_or_create(id, [&]() {
        return std::make_unique<Scene::TriangleMesh>(std::move(mesh_its));
    });
    auto geom    = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles());
    });

    Render::Material material = Render::Material{}
                                    .set_shader(
                                        m_device.context().shader_manager().shader("gouraud_light")
                                    )
                                    .set_uniform("uniform_color", color);

    builder.set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
        .set_aabb(trimesh->aabb_mesh());
}

Domain::SquareMatrix4d ndc_to_ss_inverse(const Scene::Camera& camera)
{
    const Render::Rect& viewport = camera.viewport();
    double half_w                = 0.5 * double(viewport.width);
    double half_h                = 0.5 * double(viewport.height);
    Domain::SquareMatrix4d ret;
    ret << half_w, 0.0, 0.0, double(viewport.x) + half_w, 0.0, half_h, 0.0,
        double(viewport.y) + half_h, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0;
    return ret.inverse();
}

void MeasureGizmo::build_distance_dimensioning(
    Scene::NodeBuilder& builder,
    const Domain::Vec3d& v1,
    const Domain::Vec3d& v2,
    double distance
)
{
    const Scene::Camera& camera = m_scene_presenter.scene().camera();
    Domain::Vec2d v1ss          = camera.project_to_screen_space(v1);
    Domain::Vec2d v2ss          = camera.project_to_screen_space(v2);

    if (v1ss.isApprox(v2ss))
        return;

    Domain::Vec2d v12ss = v2ss - v1ss;
    double v12ss_len    = v12ss.norm();

    bool overlap = v12ss_len - 2.0 * TRIANGLE_HEIGHT < 0.0;

    auto q12ss = Eigen::Quaternion<double>::FromTwoVectors(
        Domain::Vec3d::UnitY(),
        Domain::Vec3d(v12ss.x(), v12ss.y(), 0.0)
    );
    auto q21ss = Eigen::Quaternion<double>::FromTwoVectors(
        Domain::Vec3d::UnitY(),
        Domain::Vec3d(-v12ss.x(), -v12ss.y(), 0.0)
    );

    Domain::Vec3d v1ss_3 = {v1ss.x(), v1ss.y(), 0.0};
    Domain::Vec3d v2ss_3 = {v2ss.x(), v2ss.y(), 0.0};

    indexed_triangle_set triangle_mesh_its;
    triangle_mesh_its.vertices = {
        {0.0f, 0.0f, 0.0f},
        {-0.5f * TRIANGLE_BASE, -TRIANGLE_HEIGHT, 0.0f},
        {0.5f * TRIANGLE_BASE, -TRIANGLE_HEIGHT, 0.0f}
    };
    triangle_mesh_its.indices = {{0, 1, 2}};

    std::string id = "triangle_dimensioning";

    auto arrow_trimesh = m_triangle_mesh_manager.get_or_create(id, [&]() {
        return std::make_unique<Scene::TriangleMesh>(std::move(triangle_mesh_its));
    });
    auto arrow_geom    = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(m_device, arrow_trimesh->triangles());
    });

    Domain::SquareMatrix4d projection_view_inverse_matrix = (camera.projection() * camera.view())
                                                                .inverse();
    Domain::SquareMatrix4d ss_to_ndc_matrix = ndc_to_ss_inverse(camera);

    Render::Material material = Render::Material{}
                                    .set_shader(m_device.context().shader_manager().shader("flat"))
                                    .set_uniform("uniform_color", Domain::ColorRGBA::WHITE());

    builder.child([&](Scene::NodeBuilder& bldr) {
        Domain::Transform3d trafo = Domain::Transform3d::Identity();
        trafo.translate(v1ss_3).rotate(overlap ? q12ss : q21ss);
        trafo = projection_view_inverse_matrix * ss_to_ndc_matrix * trafo.matrix();

        bldr.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::Dimensioning})
            .set_debug_name("arrow 1")
            .set_mesh(arrow_geom, material, int(PlaterSceneLayer::AlwaysOnTop))
            .transform([&](Domain::Transform3d& xform) { xform = trafo; });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        Domain::Transform3d trafo = Domain::Transform3d::Identity();
        trafo.translate(v2ss_3).rotate(overlap ? q21ss : q12ss);
        trafo = projection_view_inverse_matrix * ss_to_ndc_matrix * trafo.matrix();

        bldr.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::Dimensioning})
            .set_debug_name("arrow 2")
            .set_mesh(arrow_geom, material, int(PlaterSceneLayer::AlwaysOnTop))
            .transform([&](Domain::Transform3d& xform) { xform = trafo; });
    });

    id = fmt::format("line_dimensioning_{}", v12ss_len);

    std::vector<Domain::Vec3f> lines = {v1ss_3.cast<float>(), v2ss_3.cast<float>()};
    const auto* stem_geom            = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_lines(m_device, lines);
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        Domain::Transform3d trafo(projection_view_inverse_matrix * ss_to_ndc_matrix);

        bldr.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::Dimensioning})
            .set_debug_name("stem")
            .set_mesh(stem_geom, material, int(PlaterSceneLayer::AlwaysOnTop))
            .transform([&](Domain::Transform3d& xform) { xform = trafo; });
    });
}

void MeasureGizmo::build_arc_edge_edge_dimensioning(
    Scene::NodeBuilder& builder,
    const SurfaceFeature& f1,
    const SurfaceFeature& f2
)
{
    DEBUG_ASSERT(f1.type() == SurfaceFeatureType::Edge && f2.type() == SurfaceFeatureType::Edge);

    double angle  = m_measurement_result.angle->angle;
    double radius = m_measurement_result.angle->radius;
    if (std::abs(angle) < Domain::EPSILON || std::abs(radius) < Domain::EPSILON)
        return;

    const Domain::Vec3d& center = m_measurement_result.angle->center;
    Domain::Vec3d e1_unit       = edge_direction(m_measurement_result.angle->e1);
    Domain::Vec3d e2_unit       = edge_direction(m_measurement_result.angle->e2);

    unsigned int resolution = std::max<unsigned int>(2, 64 * angle / double(M_PI));
    double step             = angle / double(resolution);
    Domain::Vec3d normal    = e1_unit.cross(e2_unit).normalized();

    std::vector<Domain::Vec3f> lines;
    lines.reserve(2 * resolution);
    for (unsigned int i = 0; i < resolution; ++i) {
        lines.emplace_back(
            (radius
             * (Eigen::Quaternion<double>(Eigen::AngleAxisd(step * double(i + 0), normal)) * e1_unit))
                .cast<float>()
        );
        lines.emplace_back(
            (radius
             * (Eigen::Quaternion<double>(Eigen::AngleAxisd(step * double(i + 1), normal)) * e1_unit))
                .cast<float>()
        );
    }

    std::string id = fmt::format(
        "arc_dimensioning_{}_{}_{}_{}_{}",
        radius,
        angle,
        normal.x(),
        normal.y(),
        normal.z()
    );
    const auto* geom = m_geometry_manager.get_or_create(id, [&]() {
        return Render::geometry_from_lines(m_device, lines);
    });

    Render::Material material = Render::Material{}
                                    .set_shader(m_device.context().shader_manager().shader("flat"))
                                    .set_uniform("uniform_color", Domain::ColorRGBA::WHITE());

    builder.child([&](Scene::NodeBuilder& bldr) {
        Domain::Transform3d trafo = Domain::Transform3d::Identity();
        trafo.translate(center);

        bldr.set_tag(MeasureGizmoNodeTag{MeasureGizmoElementType::Dimensioning})
            .set_debug_name("arc")
            .set_mesh(geom, material, int(PlaterSceneLayer::AlwaysOnTop))
            .transform([&](Domain::Transform3d& xform) { xform = trafo; });
    });
}

void MeasureGizmo::build_arc_edge_plane_dimensioning(
    Scene::NodeBuilder& builder,
    const SurfaceFeature& f1,
    const SurfaceFeature& f2
)
{
    DEBUG_ASSERT(f1.type() == SurfaceFeatureType::Edge && f2.type() == SurfaceFeatureType::Plane);

    const std::pair<Domain::Vec3d, Domain::Vec3d>& e1 = m_measurement_result.angle->e1;
    const std::pair<Domain::Vec3d, Domain::Vec3d>& e2 = m_measurement_result.angle->e2;
    build_arc_edge_edge_dimensioning(
        builder,
        SurfaceFeature(SurfaceFeatureType::Edge, e1.first, e1.second),
        SurfaceFeature(SurfaceFeatureType::Edge, e2.first, e2.second)
    );
}

void MeasureGizmo::build_arc_plane_plane_dimensioning(
    Scene::NodeBuilder& builder,
    const SurfaceFeature& f1,
    const SurfaceFeature& f2
)
{
    DEBUG_ASSERT(f1.type() == SurfaceFeatureType::Plane && f2.type() == SurfaceFeatureType::Plane);

    const std::pair<Domain::Vec3d, Domain::Vec3d>& e1 = m_measurement_result.angle->e1;
    const std::pair<Domain::Vec3d, Domain::Vec3d>& e2 = m_measurement_result.angle->e2;
    build_arc_edge_edge_dimensioning(
        builder,
        SurfaceFeature(SurfaceFeatureType::Edge, e1.first, e1.second),
        SurfaceFeature(SurfaceFeatureType::Edge, e2.first, e2.second)
    );
}

void MeasureGizmo::render_dimensioning()
{
    auto& scene = m_scene_presenter.scene();
    if (m_dimensioning_node != nullptr) {
        scene.remove_child(m_dimensioning_node);
        m_dimensioning_node = nullptr;
    }

    if (!m_measurement_result.has_any_data())
        return;

    Scene::NodeBuilder builder{scene};
    builder.set_debug_name("dimensioning");
    builder.set_tag(MeasureGizmoNodeTag{});

    if (m_measurement_result.has_distance_data()) {
        const DistAndPoints& dap = m_measurement_result.distance_infinite.has_value() ?
            *m_measurement_result.distance_infinite :
            *m_measurement_result.distance_strict;
        build_distance_dimensioning(builder, dap.from, dap.to, dap.dist);
    }

    if (m_measurement_result.angle.has_value()) {
        const auto& feature_cache = m_current_project->feature_cache;
        const SurfaceFeature* f1  = &feature_cache.selected[0]->feature;
        const SurfaceFeature* f2  = &feature_cache.selected[1]->feature;

        SurfaceFeatureType ft1 = f1->type();
        SurfaceFeatureType ft2 = f2->type();

        // Order features by type so following conditions are simple.
        if (ft1 > ft2) {
            std::swap(ft1, ft2);
            std::swap(f1, f2);
        }

        // If there is an angle to show, draw the arc:
        if (ft1 == SurfaceFeatureType::Edge && ft2 == SurfaceFeatureType::Edge)
            build_arc_edge_edge_dimensioning(builder, *f1, *f2);
        else if (ft1 == SurfaceFeatureType::Edge && ft2 == SurfaceFeatureType::Plane)
            build_arc_edge_plane_dimensioning(builder, *f1, *f2);
        else if (ft1 == SurfaceFeatureType::Plane && ft2 == SurfaceFeatureType::Plane)
            build_arc_plane_plane_dimensioning(builder, *f1, *f2);
    }

    auto dimensioning_node = builder.build();
    m_dimensioning_node    = dimensioning_node.get();
    scene.add_child(dimensioning_node.release(), m_main_node);
}

} // namespace Slic3r::App::Plater
