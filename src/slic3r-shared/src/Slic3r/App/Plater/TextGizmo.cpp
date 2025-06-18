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
#include <Slic3r/Domain/ModelObject.hpp> // add volume into object
#include <Slic3r/Biz/Algorithms/TriangleMesh.hpp>
#include <Slic3r/Biz/Emboss/Emboss.hpp> // also copy in libslic3r for SurfaceCut
#include <Slic3r/Biz/Emboss/EmbossJob.hpp> // embossing jobs 
#include <Slic3r/Biz/Platform/PlatformServices.hpp> // main_thread_dispatcher
#include "libslic3r/Utils.hpp"

namespace Slic3r::Biz::Emboss {
// TODO: made shape by current selected style and text
class TextShapeProvider : public ShapeProvider {

public:
    TextShapeProvider(const Domain::TextConfiguration& config, const Domain::EmbossProjection& projection)
        : m_text_configuration(config) // copy current text configuration for embossing
    {
        shape.projection = projection; // copy current projection
    }
    Domain::EmbossShape& get_shape() override {
        if (!shape.final_shape.expolygons.empty())
            return shape; // use cached value

        // create cahce
        //std::string font_path = Slic3r::resources_dir() + "/fonts/NotoSans-Regular.ttf";
        //std::unique_ptr<Domain::FontFile> font_ptr = create_font_file(font_path.c_str());

        Biz::Platform::IFontManager& font_manager = Biz::Platform::PlatformServices::instance().font_manager();        
        FontFileWithCache font_with_cache(font_manager.open(m_text_configuration.style.descriptor));
        std::wstring text = boost::nowide::widen(m_text_configuration.text);
        const Domain::FontProp font_prop; // default font properties
        shape = {.shapes_with_ids{text2vshapes(font_with_cache, text, font_prop)}};
        return shape;
    }
    void write(Domain::ModelVolume& volume) const override {
        ShapeProvider::write(volume);
        volume.text_configuration = m_text_configuration; // copy
        assert(volume.emboss_shape.has_value());

        // Fix for object: stored attribute that volume is embossed per glyph when it is object
        if (m_text_configuration.style.prop.per_glyph && volume.is_the_only_one_part())
            volume.text_configuration->style.prop.per_glyph = false;
    }
private:
    // font item is not used for create object
    Domain::TextConfiguration m_text_configuration;
};
}

namespace{
using namespace Slic3r;

bool is_selected_object(const Biz::Scene::Selection::ElementRefs& selected_elements) {
    if (selected_elements.empty()) 
        return false;   
    return true;
}

Biz::Emboss::CreateVolumeParams create_volume_params(
    Biz::ProjectInteractor& project_interactor, 
    Domain::TextConfiguration& configuration,
    Domain::ModelVolumeType volume_type = Domain::ModelVolumeType::MODEL_PART) {
    Domain::EmbossProjection projection{
        .depth = 5.f,
        .use_surface = false
    };
    return Biz::Emboss::CreateVolumeParams{
        .base{
            .shape_provider = std::make_unique<Biz::Emboss::TextShapeProvider>(
                configuration, projection),
            .project_interactor = project_interactor,
            .is_outside = (volume_type == Domain::ModelVolumeType::MODEL_PART),
            .volume_name = "Embossed textik"
        },
        .volume_type = volume_type,
        .gizmo = 13
    };
}

}

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
        auto params = create_volume_params(m_project_interactor, m_text_configuration, Domain::ModelVolumeType::NEGATIVE_VOLUME);
        if (Biz::Emboss::start_create_volume(params, ctx.pick_ray(), ctx.pick_results()))
            return Scene::GizmoActivationState::Active; // create volume at pick ray
    }
    return Scene::GizmoActivationState::Inactive;
}

void TextGizmo::register_commands(Platform::CommandRegistry& registry) {
    registry.register_command(std::make_unique<Platform::FuncCommand>(
        "Create/Edit text", [&]() { add_text_by_view_direction(Domain::ModelVolumeType::MODEL_PART); }, nullptr,
        Platform::KeyboardShortcut{0, Platform::KeyCode::T}
    ));
}

void TextGizmo::render_imgui()
{
    if (ImGui::Begin("Text Gizmo")) {
        ImGui::TextColored(ImVec4(.1f, .9f, .2f, 1.f), "RClick add negative volume \n or object on plate");
        
        Biz::Platform::IFontManager& font_manager = Biz::Platform::PlatformServices::instance().font_manager();
        const Domain::FontList& fonts = font_manager.get_fonts();
        auto it_font = std::find_if(fonts.begin(), fonts.end(),
            [&name = m_text_configuration.style.descriptor.name](const Domain::FontDescriptor& fd) {
                return fd.name == name; });

        std::string selected = (it_font == fonts.end()) ? std::string("Not selected yet") : it_font->name;
        if (ImGui::BeginCombo("Font", selected.c_str()))
        {
            for (const Domain::FontDescriptor &fd: fonts)
            {
                const bool is_selected = (it_font == fonts.end())? false : &fd == &(*it_font);
                if (ImGui::Selectable(fd.name.c_str(), is_selected)) {
                    m_text_configuration.style.descriptor = fd;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Emboss text");
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

bool TextGizmo::add_text_by_view_direction(Domain::ModelVolumeType volume_type) {
    if (m_gizmo_manager.current_tool_type() == type())
        return false; // already active

    if (!init_create(volume_type))
        return false;

    auto params = create_volume_params(m_project_interactor, m_text_configuration, volume_type);
    return Biz::Emboss::start_create_volume_without_position(params);
}

void TextGizmo::close() { m_gizmo_manager.deactivate_current_tool();}

bool TextGizmo::init_create(Domain::ModelVolumeType volume_type) {
    if (volume_type != Domain::ModelVolumeType::MODEL_PART &&
        volume_type != Domain::ModelVolumeType::NEGATIVE_VOLUME &&
        volume_type != Domain::ModelVolumeType::PARAMETER_MODIFIER)
        return false; // invalid volume type for emboss text

    // if (wxGetApp().obj_list()->has_selected_cut_object()) return false;
    return true;
}

} // namespace Slic3r::App::Plater
