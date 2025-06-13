///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextGizmo.hpp"
#include "Slic3r/App/Plater/TextDialog.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

using namespace Slic3r::App::Yoga;
#include <boost/nowide/convert.hpp>
#include <imgui/imgui.h>

#include <Slic3r/Domain/TriangleMesh.hpp>
#include <Slic3r/Biz/Algorithms/TriangleMesh.hpp>
#include <Slic3r/Biz/Emboss/Emboss.hpp> // also copy in libslic3r for SurfaceCut
#include "Slic3r/Biz/Platform/PlatformServices.hpp" // main_thread_dispatcher
#include <Slic3r/App/Scene/BedNodeTag.hpp>
#include <Slic3r/App/Plater/SceneNodeTag.hpp>

namespace {
using namespace Slic3r;
using namespace Slic3r::Biz::Emboss;

using Slic3r::Biz::Platform::IMainThreadDispatcher;
using Slic3r::Biz::Platform::PlatformServices; 
using Slic3r::App::Scene::Ray;

Domain::TriangleMesh create_mesh() {

    std::string font_path = Slic3r::resources_dir() + "/fonts/NotoSans-Regular.ttf";
    std::unique_ptr<FontFile> font_ptr = create_font_file(font_path.c_str());
    FontFileWithCache font_with_cache(std::move(font_ptr));
    std::wstring text = boost::nowide::widen("Emboss text");
    const Domain::FontProp font_prop; // default font properties
    Domain::EmbossShape emboss_shape {
        .shapes_with_ids = text2vshapes(font_with_cache, text, font_prop)};

    auto projectZ = std::make_unique<ProjectZ>(409600);
    Transform3d tr = Eigen::Translation<double, 3>(0., 0., 1.) * Eigen::Scaling(4.8828125e-06);
    ProjectTransform project(std::move(projectZ), tr);
    ExPolygons text_shape = union_with_delta(emboss_shape, UNION_DELTA, UNION_MAX_ITERATIN);
    indexed_triangle_set its = polygons2model(text_shape, project);
    return Biz::Algorithms::TriangleMesh::construct(its);
}

bool create_object(Biz::ProjectInteractor& project_interactor, const Vec2d& z0_coor)
{
    // To check if the project is same
    Domain::SelectionId project_id = project_interactor.selected_project_id();        
    Domain::TriangleMesh mesh = create_mesh();
    double z_move = -mesh.bounding_box().min.z();

    Biz::Scene::SceneInteractor& scene_interactor = project_interactor.scene_interactor();
    scene_interactor.new_object_from_mesh(std::move(mesh), project_id);
    scene_interactor.transform_selection(Transform3d(
        Eigen::Translation<double, 3>(z0_coor.x(), z0_coor.y(), z_move)
    ).matrix());
    return true;
}

bool create_object(Biz::ProjectInteractor& project_interactor, const Ray& ray){
    double d_z = ray.direction.z();
    if (fabs(d_z) - 1e-4 > 0.) { // not parallel to Z axis
        Vec3d z0 = ray.point_at(-ray.origin.z() / d_z);
        Vec2d bed_coor(z0.x(), z0.y());
        create_object(project_interactor, bed_coor);
        // m_gizmo_manager.activate_tool(type(), ptAny);
        // const Domain::Bed& bed =
        // m_project_interactor.selected_project().config_containers().front()->bed();
        return true;
    }

    return false;
}

bool is_selected_object(const Biz::Scene::Selection::ElementRefs& selected_elements) {
    if (selected_elements.empty()) 
        return false;
    
    return true;
}

//ModelVolume* add_volume_from_mesh(
//    const Domain::ObjectID& object_id,
//    Domain::TriangleMesh&& mesh,
//    ModelVolumeType type = ModelVolumeType::MODEL_PART,
//    const Transform& trafo = Matrix4d::Identity())
//{
//    
//}

struct CreateVolumeData {
    // To inform application listener about volume creation
    // NOTE1: Before access check thread cancel
    // NOTE2: Current project do not have to be one where add new volume
    Biz::ProjectInteractor& project_interactor; // live longer than TextGizmo (@barzto garanteed)
    Domain::SelectionId project_id = Domain::INVALID_ID; // project id, where to create volume
    Domain::ObjectID object_id = Domain::INVALID_ID; // object id, where to create volume    
    ModelVolumeType volume_type = ModelVolumeType::MODEL_PART; // type of volume to create

    Transform3d tr = Transform3d::Identity();
    std::string name = "Embossed text"; // default name for volume
};

bool create_volume(CreateVolumeData& data) {

    /* TODO: find way to add volume into not selected project
    auto& project = data.project_interactor.workbench().project(data.project_id);
    /*/
    auto& project = data.project_interactor.selected_project();
    ASSERT(data.project_interactor.selected_project_id() == data.project_id); // project does not match
    // */

    // create volume
    Domain::TriangleMesh mesh = create_mesh();
    

    /* only add volume without addition data
    scene_interactor.add_volume_from_mesh(std::move(mesh), volume_type, tr.matrix());
    /*/
    auto& obj = *project.find_object_by_id(data.object_id.id);
    auto* vol = obj.add_volume(std::move(mesh), data.volume_type);
    vol->set_transformation(data.tr);
    vol->name = data.name;

    data.project_interactor.scene_interactor().add_volume(vol);
    // */
    return true;

}

// Inspired in Biz::SceneInteractor::add_volume_from_mesh()
bool create_volume(
    Biz::ProjectInteractor& project_interactor, 
    Slic3r::ModelVolumeType volume_type){
    const Biz::Scene::Selection& sel = project_interactor.scene_interactor().selection();
    if (sel.elements.empty())
        return false; // no object selected

    CreateVolumeData data{
        .project_interactor = project_interactor,
        .project_id = project_interactor.selected_project_id(),
        .object_id = sel.elements[0].object_id,
        .volume_type = volume_type
    };
    return create_volume(data);
}

} // namespace

namespace Slic3r::App::Plater {
TextGizmo::TextGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor& project_interactor,
    Scene::GizmoManager& gizmo_manager
)
    : m_device(device)
    , m_scene_presenter(scene_presenter)
    , m_project_interactor(project_interactor)
    , m_gizmo_manager(gizmo_manager)
{
    m_dialog = std::make_unique<TextDialog>();

    m_dialog->callbacks().editor_text_changed = [](const std::string& new_text) {
        // do something with new text
    };

    m_dialog->callbacks().save_preset_as = [this]() {
        m_dialog->set_enable_line_gap(true); // test
        m_dialog->update_units(false); // test
        };
    m_dialog->callbacks().save_preset = [this]() {
        m_dialog->set_warning("There is something wrong!!!\ndfghjkl"); // test
    };
    m_dialog->callbacks().rename_preset = [this]() {
        m_dialog->set_warning(""); // test
    };
    m_dialog->callbacks().delete_preset = [this]() {
        m_dialog->show_revert_buttons(true); // test
    };
    m_dialog->callbacks().set_on_face_camera = [this]() {
        m_dialog->show_revert_buttons(false); // test
    };

    m_dialog->callbacks().preset_selection_changed = [this](int id) {
        };
    m_dialog->callbacks().font_selection_changed = [this](int id) {
        };
    m_dialog->callbacks().style_selection_changed = [this](int id) {
        };
    m_dialog->callbacks().operation_selection_changed = [this](int id) {
        };
}

bool TextGizmo::enabled() const { return true; };
Scene::ToolType TextGizmo::type() const { return Scene::ToolType::TextGizmo; }

Yoga::GizmoWindowPtr TextGizmo::release_ui_window()
{
    return m_dialog.release();
}

void TextGizmo::update_layout(bool show_for_part)
{
    m_dialog->show_part_specific_panel(show_for_part);
}
Scene::GizmoActivationState TextGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active){
    using App::Platform::MouseEvent;
    using App::Platform::MouseButton;
    const MouseEvent& mouse_event = ctx.mouse_event();
    if (mouse_event.type() == MouseEvent::Type::ButtonDown &&
        mouse_event.button() == MouseButton::Right) {
        if (create_volume(ModelVolumeType::MODEL_PART, ctx.pick_ray(), ctx.pick_results()))
            return Scene::GizmoActivationState::Active; // create volume at pick ray
    }
    return Scene::GizmoActivationState::Inactive;
}

void TextGizmo::register_commands(Platform::CommandRegistry& registry) {
    registry.register_command(std::make_unique<Platform::FuncCommand>(
        "Create/Edit text", [&]() { add_text_by_view_direction(ModelVolumeType::MODEL_PART); }, nullptr,
        Platform::KeyboardShortcut{0, Platform::KeyCode::T}
    ));
}

void TextGizmo::render_imgui()
{
    ImGui::TextColored(ImVec4(.1f, .9f, .2f, 1.f), "RClick add negative volume \n or object on plate");
    if (ImGui::Begin("Text Gizmo")) {
        ImGui::Text("Emboss text");
        if (ImGui::Button("Add Object on[0,0]")) {
            create_object(m_project_interactor, Vec2d(0,0));
        }
        if (ImGui::Button("Add negative Volume")) {
            ::create_volume(m_project_interactor, Slic3r::ModelVolumeType::NEGATIVE_VOLUME);
        }
        if (ImGui::Button("Close")) {
            close();
        }
    }
    ImGui::End();
}

void TextGizmo::on_activated()
{
    std::vector<std::string> presets = { "NORMAL", "SMALL", "ITALIC", "SWISS" };
    int selected_preset_id = 2;
    m_dialog->set_presets(presets, selected_preset_id);

    // load current font_preset
    activate_preset(/*font_preset*/);

    bool use_inch = true; // wxGetApp().app_config->get_bool("use_inches");
    m_dialog->update_units(use_inch);
    m_dialog->set_enable_all_except_font(true); // test
}

void TextGizmo::on_deactivated() {}

bool TextGizmo::add_text_by_view_direction(Slic3r::ModelVolumeType volume_type) {
    if (m_gizmo_manager.current_tool_type() == type())
        return false; // already active

    if (!init_create(volume_type))
        return false;

    // is selected object
    Biz::Scene::SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
    const Biz::Scene::Selection& selection = scene_interactor.selection();
    if (!is_selected_object(selection.elements)) {
        const Scene::Camera& camera = m_scene_presenter.scene().camera();
        const Render::Rect& rect = camera.viewport();
        // TODO: subtract sides menu size
        Scene::Ray ray = camera.ray_at(rect.width / 2., rect.height / 2.);
        ::create_object(m_project_interactor, ray); // create object at center of screen
        m_gizmo_manager.activate_tool(type(), ptFFF);
    }
    return ::create_volume(m_project_interactor, volume_type);
}

bool TextGizmo::create_volume(Slic3r::ModelVolumeType volume_type, const Scene::Ray& pick_ray, const Scene::NodePickResults& picks) {
    const Domain::Project& project = m_project_interactor.selected_project();
    for (const Scene::NodePickResult& pick : picks) {
        if (pick.node->has_tag_of_type<App::Plater::SceneNodeTag>()) {
            auto* tag = pick.node->tag_of_type<App::Plater::SceneNodeTag>();
            const ModelVolume* volume = project.find_volume_by_id(tag->object_id, tag->volume_id);
            if (volume == nullptr)
                continue; // no volume under mouse
            // TODO: What to do with Negative volume
            if (volume->type() != ModelVolumeType::MODEL_PART)
                continue; // skip modifiers + SupportBlock/Enforce

            double UP_LIMIT = 0.9;
            Vec3d pick_point = pick_ray.point_at(pick.cast.distance);
            Vec3d pick_normal = pick.cast.normal;
            const ModelInstance* instance = project.find_instance_by_id(tag->object_id, tag->instance_id);
            Transform3d surface_trmat = create_transformation_onto_surface(pick_point, pick_normal, UP_LIMIT);
            Transform3d tr = instance->get_matrix().inverse() * surface_trmat;
            
            CreateVolumeData data{
                .project_interactor = m_project_interactor,
                .project_id = m_project_interactor.selected_project_id(),
                .object_id = volume->get_object()->id(),
                .volume_type = ModelVolumeType::NEGATIVE_VOLUME, //volume_type,
                .tr = tr,
                .name = "Embossed text Volume"
            };
            return ::create_volume(data);
        }
        if (pick.node->has_tag_of_type<App::Scene::BedNodeTag>())
            return ::create_object(m_project_interactor, pick_ray);
    }
    return add_text_by_view_direction(volume_type); // fall back, do not use pick ray
}

void TextGizmo::close() { m_gizmo_manager.deactivate_current_tool();}


bool TextGizmo::init_create(Slic3r::ModelVolumeType volume_type) { 
    if (volume_type != ModelVolumeType::MODEL_PART &&
        volume_type != ModelVolumeType::NEGATIVE_VOLUME &&
        volume_type != ModelVolumeType::PARAMETER_MODIFIER)
        return false; // invalid volume type for emboss text

    // if (wxGetApp().obj_list()->has_selected_cut_object()) return false;
    return true;
}

} // namespace Slic3r::App::Plater
