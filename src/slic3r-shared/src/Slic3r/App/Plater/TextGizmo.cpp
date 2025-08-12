///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextGizmo.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Plater/TextDialog.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <boost/nowide/convert.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h> // using std::string for inputs

#include <Slic3r/Domain/TriangleMesh.hpp>
#include <Slic3r/Domain/ModelObject.hpp> // add volume into object
#include <Slic3r/Biz/Algorithms/TriangleMesh.hpp>
#include <Slic3r/Biz/Emboss/Emboss.hpp> // also copy in libslic3r for SurfaceCut
#include <Slic3r/Biz/Emboss/EmbossJob.hpp> // embossing jobs
#include <Slic3r/Biz/Platform/PlatformServices.hpp> // main_thread_dispatcher
#include "libslic3r/Utils.hpp"

using namespace Slic3r::App::Yoga;
namespace Slic3r::Biz::Emboss {
// TODO: made shape by current selected preset and text
class TextShapeProvider : public ShapeProvider
{
public:
    TextShapeProvider(
        const Domain::TextConfiguration& text_configuration,
        const Domain::EmbossProjection& projection,
        Biz::Emboss::IFontManager& font_manager
    ) 
        : m_text_configuration(text_configuration)
        , m_font_manager(font_manager)
    {
        shape.projection = projection; // copy current projection
    }

    Domain::EmbossShape& get_shape() override
    {
        if (!shape.final_shape.expolygons.empty())
            return shape; // use cached value
        FontFileWithCache font_with_cache(m_font_manager.open(m_text_configuration.style.descriptor));
        std::wstring text = boost::nowide::widen(m_text_configuration.text);
        const Domain::FontProp& font_prop = m_text_configuration.style.prop;
        shape.shapes_with_ids = text2vshapes(font_with_cache, text, font_prop);
        return shape;
    }

    void write(Domain::ModelVolume& volume) const override
    {
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
    Biz::Emboss::IFontManager& m_font_manager;
};
} // namespace Slic3r::Biz::Emboss

namespace Slic3r::App::Plater {

TextGizmo::TextGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor& project_interactor,
    Biz::Emboss::IFontManager& font_manager,
    Scene::GizmoManager& gizmo_manager
) :
    m_device(device),
    m_scene_presenter(scene_presenter),
    m_project_interactor(project_interactor),
    m_font_manager(font_manager),
    m_gizmo_manager(gizmo_manager),
    m_preset_manager(
        font_manager,
        ImGui::GetIO().Fonts->GetGlyphRangesDefault(),
        data_dir() + "/text_emboss_presets.cereal"
    )
{
    // Initialize font descriptor to font copied with application
    m_preset_manager.get_preset().emboss_style.descriptor = Domain::FontDescriptor{
        .name = "Prusa-slic3r font",
        .path = Slic3r::resources_dir() + "/fonts/NotoSans-Regular.ttf",
        .type = Domain::FontDescriptor::Type::file_path
    };

    // Dialog callback settings (order follow UI)
    m_dialog = std::make_unique<TextDialog>();
    m_dialog->callbacks().text_changed = [this](const std::string& text) { m_text = text; };
    m_dialog->callbacks().font_selection_changed =
        [this](const Domain::FontDescriptor& font_descriptor) {
            m_preset_manager.get_preset().emboss_style.descriptor.path = font_descriptor.path;
        };
    // style is only subcategory of font
    m_dialog->callbacks().height_changed = [this](double value) {
        m_preset_manager.get_font_prop().size_in_mm = value;
        };
    m_dialog->callbacks().depth_changed = [this](double value) {
        m_preset_manager.get_preset().projection.depth = value;
        };
    m_dialog->callbacks().save_preset_as = [this]() {
        m_preset_manager.save_preset_as(); 
        m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index()); };
    m_dialog->callbacks().save_preset = [this]() { m_preset_manager.store_presets(); };
    m_dialog->callbacks().rename_preset = [this]() {
        m_preset_manager.rename_preset(); 
        m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index()); };
    m_dialog->callbacks().delete_preset = [this]() { 
        if (m_preset_manager.delete_preset()) {
            activate_preset();
            m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index());
        }
    };

    m_dialog->callbacks().set_on_face_camera = [this]() { 
        m_dialog->set_enable_line_gap(false); // test
        };

    m_dialog->callbacks().preset_selection_changed = [this](int id) {
        m_preset_manager.load_preset(static_cast<size_t>(id));
        activate_preset();
    };
    m_dialog->callbacks().operation_selection_changed = [this](int id) {};
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

Scene::GizmoActivationState TextGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    using App::Platform::MouseButton;
    using App::Platform::MouseEvent;
    const MouseEvent& mouse_event = ctx.mouse_event();
    if (mouse_event.type() == MouseEvent::Type::ButtonDown
        && mouse_event.button() == MouseButton::Right)
    {
        Domain::ModelVolumeType type = Domain::ModelVolumeType::NEGATIVE_VOLUME;
        if(emboss_text(type, ctx.pick_ray(), ctx.pick_results()))
            return Scene::GizmoActivationState::Active; // create volume at pick ray
    }
    return Scene::GizmoActivationState::Inactive;
}

void TextGizmo::register_commands(Platform::CommandRegistry& registry)
{
    registry.register_command(
        std::make_unique<Platform::FuncCommand>(
            "Create/Edit text",
            [&]() { add_text_by_view_direction(Domain::ModelVolumeType::MODEL_PART); },
            nullptr,
            Platform::KeyboardShortcut{0, Platform::KeyCode::T}
        )
    );
}

void TextGizmo::render_imgui()
{
    if (!ImGui::Begin("Text Gizmo"))
        return ImGui::End();
     
    ImGui::TextColored(
        ImVec4(.1f, .9f, .2f, 1.f),
        "RClick add negative volume \n or object on plate"
    );

    ImVec2 input_size(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 3);
    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput
        | ImGuiInputTextFlags_AutoSelectAll;
    ImGui::InputTextMultiline("##emboss_text_input", &m_text, input_size, flags);

    const Domain::FontList& fonts = m_font_manager.get_fonts();
    auto it_font                  = std::find_if(
        fonts.begin(),
        fonts.end(),
        [&path = m_preset_manager.get_preset().emboss_style.descriptor.path](const Domain::FontDescriptor& fd) {
            return fd.path == path;
        }
    );
    std::string selected = (it_font == fonts.end()) ? std::string("Not selected yet") :
                                                        it_font->name;
    if (ImGui::BeginCombo("Font", selected.c_str())) {
        for (const Domain::FontDescriptor& fd : fonts) {
            const bool is_selected = (it_font == fonts.end()) ? false : &fd == &(*it_font);
            if (ImGui::Selectable(fd.name.c_str(), is_selected)) {
                m_preset_manager.get_preset().emboss_style.descriptor = fd;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // pressets

    if (ImGui::BeginCombo(
            "Pressets",
            m_preset_manager.get_preset().emboss_style.descriptor.name.c_str()
        ))
    {
        const auto& styles = m_preset_manager.get_presets();
        for (const Biz::Emboss::TextPresetManager::Preset& style : styles) {
            const bool is_selected = (&style - &styles.front())
                == m_preset_manager.get_preset_index();
            if (ImGui::Selectable(style.emboss_style.descriptor.name.c_str(), is_selected)) {
                m_preset_manager.load_preset(style);
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
        
    ImGui::InputFloat("size_in_mm", &m_preset_manager.get_preset().emboss_style.prop.size_in_mm);
    ImGui::InputDouble("depth", &m_preset_manager.get_preset().projection.depth);

    if (ImGui::Button("Close")) {
        close();
    }

    ImGui::End();
}

namespace {
size_t get_index(const Domain::FontList& fonts, const std::string& path)
{
    auto it_font = std::find_if(
        fonts.begin(),
        fonts.end(),
        [&path](const Domain::FontDescriptor& fd) {
            return fd.path == path;
        }
    );
    return (it_font == fonts.end()) ? 0 : (it_font - fonts.begin());
}

const Domain::ModelVolume* get_selected_text_volume(const Biz::ProjectInteractor& project_interactor) {
    const Biz::Scene::SceneInteractor& scene_interactor = project_interactor.scene_interactor();
    const Biz::Scene::ObjectSelection& selection = scene_interactor.object_selection();
    if (selection.elements.size() != 1)
        return nullptr; // multiple volumes selected

    const Domain::ElementRef& selected = selection.elements.front();
    const Domain::Project& project = project_interactor.selected_project();
    
    const Domain::ModelVolume* volume_ptr = nullptr;
    if (selected.has_volume()) {
        volume_ptr = project.find_volume_by_id(selected.object_id, selected.volume_id);    
    } else {
        // Check is selected object contain only volume with text
        const Domain::ModelObject* object_ptr = project.find_object_by_id(selected.object_id);
        if (object_ptr->volumes.size() != 1)
            return nullptr;
        volume_ptr = object_ptr->volumes.front();
    }

    if (volume_ptr == nullptr)
        return nullptr;

    if (!volume_ptr->text_configuration.has_value())
        return nullptr; // selected volume is not text

    return volume_ptr;
}
}

void TextGizmo::on_activated()
{
    if (m_preset_manager.get_presets().empty())
        m_preset_manager.init();

    const Domain::ModelVolume* volume_ptr = get_selected_text_volume(m_project_interactor);
    if (volume_ptr == nullptr) {
        // create new text volume
        m_preset_manager.discard_preset_changes();
        m_text = "Emmmbosss text";

        // What shows few miliseconds till new item is created?
        m_dialog->set_enable_all_except_font(false);

        add_text_by_view_direction(Domain::ModelVolumeType::MODEL_PART);
        // TODO: how to wait till it is created?
        // Catch selection changed event
    } else {
        m_dialog->set_enable_all_except_font(true);
        // load current settings
        const Domain::TextConfiguration& tc = *volume_ptr->text_configuration;
        const Domain::EmbossStyle& style = tc.style;

        Biz::Emboss::TextPresetManager::Preset preset{
            .emboss_style = style,
            .projection = volume_ptr->emboss_shape->projection
            // .distance = calc_distance(),
            // .angle = calc_angle(selection)
        };

        const auto& presets = m_preset_manager.get_presets();
        auto preset_it = std::find_if(presets.begin(), presets.end(),
            [&name = style.descriptor.name](const Biz::Emboss::TextPresetManager::Preset& preset) {
                return preset.emboss_style.descriptor.name == name;
            });

        if (preset_it == presets.end()) {
            // unknown preset inside volume, create temporary one
            m_preset_manager.load_preset(preset);
        } else {
            m_preset_manager.load_preset(presets.begin() - preset_it);
            m_preset_manager.get_preset() = preset;                
        }
        // TODO: update dialog
        
        // Do not use focused input value when switch volume(it must swith value)
        //ImGuiPureWrap::left_inputs();
    }

    m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index());

    // load current font_preset
    activate_preset(/*font_preset*/);

    bool use_inch = false; // wxGetApp().app_config->get_bool("use_inches");
    m_dialog->update_units(use_inch);

    // Propadate reloaded installed font into the dialog
    // NOTE: reload fonts from OS, 2.9.2 do it on dialog open, now it is on gizmo activation
    const Domain::FontList& fonts = m_font_manager.get_fonts(); // Re-Load Os fonts
    m_dialog->set_fonts(fonts);
    if (m_preset_manager.exist_stored_style())
        m_dialog->set_font(m_preset_manager.get_stored_preset()->emboss_style.descriptor, true);
    m_dialog->set_font(m_preset_manager.get_preset().emboss_style.descriptor, false);
}

void TextGizmo::on_deactivated() {}

Scene::ToolType TextGizmo::type() const {
    return Scene::ToolType::Text;
}

bool TextGizmo::add_text_by_view_direction(Domain::ModelVolumeType volume_type)
{
    //if (m_gizmo_manager.current_tool_type() == type())
    //    return false; // already active

    if (!init_create(volume_type))
        return false;    

    Scene::ISceneProvider& scene_provider = m_scene_presenter;

    // get (pickray + pickresults) from screen center
    Scene::Scene& scene = static_cast<Scene::ISceneProvider&>(m_scene_presenter).scene();
    const Render::Rect& v = scene.camera().viewport();
    Domain::Point logic_center{ v.x + v.width / 2, v.y + v.height / 2 };
    Scene::NodePickResults pick_results;
    Scene::Ray pick_ray;
    scene.pick_at(logic_center.x(), logic_center.y(), pick_results, &pick_ray);

    return emboss_text(volume_type, pick_ray, pick_results);
}

void TextGizmo::close()
{
    m_gizmo_manager.deactivate_current_tool();
}

bool TextGizmo::init_create(Domain::ModelVolumeType volume_type)
{
    if (volume_type != Domain::ModelVolumeType::MODEL_PART
        && volume_type != Domain::ModelVolumeType::NEGATIVE_VOLUME
        && volume_type != Domain::ModelVolumeType::PARAMETER_MODIFIER)
        return false; // invalid volume type for emboss text

    // if (wxGetApp().obj_list()->has_selected_cut_object()) return false;
    return true;
}

namespace {
    using namespace Slic3r;

    Biz::Emboss::CreateVolumeParams create_volume_params(
        Biz::ProjectInteractor& project_interactor,
        Biz::Emboss::IFontManager& font_manager,
        Domain::TextConfiguration&& configuration,
        Domain::EmbossProjection& projection,
        Domain::ModelVolumeType volume_type = Domain::ModelVolumeType::MODEL_PART
    )
    {
        Domain::SelectionId project_id = project_interactor.selected_project_id();
        return Biz::Emboss::CreateVolumeParams{
            .base{
                .shape_provider = std::make_unique<Biz::Emboss::TextShapeProvider>(
                    std::move(configuration),
                    projection,
                    font_manager
                ),
                .project_interactor = project_interactor,
                .project_id = project_id,
                .gizmo = static_cast<uint8_t>(App::Scene::ToolType::Text),
                .is_outside = (volume_type == Domain::ModelVolumeType::MODEL_PART),
                .volume_name = "Embossed textik"
            },
            .volume_type = volume_type
        };
    }
} // namespace

bool TextGizmo::emboss_text(Domain::ModelVolumeType volume_type, const Scene::Ray& ray, const Scene::NodePickResults& results)
{
    Domain::TextConfiguration text_config{
        .style = m_preset_manager.get_preset().emboss_style,
        .text = m_text
    };
    auto params = create_volume_params(
        m_project_interactor,
        m_font_manager,
        std::move(text_config),
        m_preset_manager.get_preset().projection,
        volume_type
    );
    return Biz::Emboss::start_create_volume(params, ray, results);
}

void TextGizmo::update_presets_list() {}

void TextGizmo::activate_preset(/*preset*/)
{    
    bool exist_stored = m_preset_manager.exist_stored_style();
    const Biz::Emboss::TextPresetManager::Preset& preset = m_preset_manager.get_preset();
    const Biz::Emboss::TextPresetManager::Preset& preset_ = exist_stored ?
        *m_preset_manager.get_stored_preset() : preset;

    m_dialog->set_editor(m_text);

    // TODO: solve conversion from font name
    const Domain::EmbossStyle& es = preset.emboss_style;
    const Domain::EmbossStyle& es_ = preset_.emboss_style;
    if (exist_stored)
        m_dialog->set_font(es_.descriptor, true);
    m_dialog->set_font(es.descriptor, false);

    const Domain::FontProp& prop = es.prop;
    const Domain::FontProp& prop_ = es_.prop;
    double height_from = 0.1;
    double height_to = 100.;
    double height_step = 0.1;
    double height_step_fast = 1;
    double height = prop.size_in_mm;
    double height_default = prop_.size_in_mm;
    m_dialog->set_height(height_from, height_to, height_step, height_step_fast, height, height_default);

    const Domain::EmbossProjection& ep = preset.projection;
    const Domain::EmbossProjection& ep_ = preset_.projection;

    double depth_from = 0.1;
    double depth_to = 100.;
    double depth_step = 0.1;
    double depth_step_fast = 1;
    m_dialog->set_depth(depth_from, depth_to, depth_step, depth_step_fast, ep.depth, ep_.depth);
    m_dialog->set_use_surface(ep.use_surface, ep_.use_surface);
    m_dialog->set_per_glyph(prop.per_glyph, prop_.per_glyph);
    m_dialog->set_align(prop.align, prop_.align);

    double scale = 1e-3; // font points to mm
    double char_gap_max = 3.62;
    double char_gap_step = 0.01;
    double char_gap_in_mm = prop.char_gap.value_or(0) * scale;
    double char_gap_in_mm_ = prop_.char_gap.value_or(0) * scale;
    m_dialog->set_char_gap(char_gap_max, char_gap_step, char_gap_in_mm, char_gap_in_mm_);

    double line_gap_max = 3.62;
    double line_gap_step = 0.01;
    double line_gap_in_mm = prop.line_gap.value_or(0) * scale;
    double line_gap_in_mm_ = prop_.line_gap.value_or(0) * scale;
    m_dialog->set_line_gap(line_gap_max, line_gap_step, line_gap_in_mm, line_gap_in_mm_);

    double boldness_max = 0.8;
    double boldness_step = 0.1;
    double boldness_in_mm = prop.boldness.value_or(0) * scale;
    double boldness_in_mm_ = prop_.boldness.value_or(0) * scale;
    m_dialog->set_boldness(boldness_max, boldness_step, boldness_in_mm, boldness_in_mm_);

    double skew_ratio_max = 1.;
    double skew_ratio_step = 0.01;
    double skew_ratio = prop.skew.value_or(0.f);
    double skew_ratio_ = prop_.skew.value_or(0.f);
    m_dialog->set_skew_ratio(skew_ratio_max, skew_ratio_step, skew_ratio, skew_ratio_);

    double surface_distance_max = 2.;
    double surface_distance_step = 0.01;
    double surface_distance = 0.;
    double surface_distance_ = preset_.distance.value_or(0.f);
    m_dialog->set_surface_distance(surface_distance_max, surface_distance_step, surface_distance, surface_distance_);
    bool allowe_surface_distance = !preset.projection.use_surface;// && !m_volume->is_the_only_one_part();
    m_dialog->set_enable_surface_distance(allowe_surface_distance);

    double rotation_max = 180.;
    double rotation_step = 0.1;
    double rotation = 92.;
    double rotation_ = preset_.angle.value_or(0.f);
    m_dialog->set_rotation(rotation_max, rotation_step, rotation, rotation_);    
}

} // namespace Slic3r::App::Plater
