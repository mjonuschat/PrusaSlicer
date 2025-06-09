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

#include <Slic3r/Domain/TriangleMesh.hpp>
#include <Slic3r/Biz/Algorithms/TriangleMesh.hpp>
#include <Slic3r/Biz/Emboss/Emboss.hpp> // also copy in libslic3r for SurfaceCut

namespace {
using namespace Slic3r;
using namespace Slic3r::Biz::Emboss;

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

void create_object(Biz::ProjectInteractor& project_interactor)
{
    auto& scene_interactor = project_interactor.scene_interactor();
    const auto& bed = project_interactor.selected_project().config_containers().front()->bed();
    scene_interactor.new_object_from_mesh(create_mesh());

    Transform3d xform = Transform3d::Identity();
    xform.translate(Vec3d{bed.center().x(), bed.center().y(), 0});
    scene_interactor.transform_selection(xform.matrix());
}

} // namespace

namespace Slic3r::App::Plater {
TextGizmo::TextGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor& project_interactor,
    CloseFn close_fn
)
    : m_device(device)
    , m_scene_presenter(scene_presenter)
    , m_project_interactor(project_interactor)
    , m_close_fn(close_fn)
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
    return Scene::GizmoActivationState::Inactive;
}

void TextGizmo::render_imgui() const
{
    if (ImGui::Begin("Text Gizmo")) {
        ImGui::Text("Emboss text");
        if (ImGui::Button("Add Object")) {
            create_object(m_project_interactor);
        }
        if (ImGui::Button("Close")) {
            m_close_fn();
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

} // namespace Slic3r::App::Plater
