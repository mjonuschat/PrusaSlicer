///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextGizmo.hpp"
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
        const Domain::FontProp font_prop; // default font properties
        shape = {.shapes_with_ids{text2vshapes(font_with_cache, text, font_prop)}};
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
    
    return Biz::Emboss::CreateVolumeParams{
        .base{
            .shape_provider = std::make_unique<Biz::Emboss::TextShapeProvider>(
                std::move(configuration),
                projection,
                font_manager
            ),
            .project_interactor = project_interactor,
            .is_outside         = (volume_type == Domain::ModelVolumeType::MODEL_PART),
            .volume_name        = "Embossed textik"
        },
        .volume_type = volume_type,
        .gizmo       = 13
    };
}
} // namespace

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
        data_dir() + "/cache/emboss_presets.cereal"
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
    m_dialog->callbacks().text_changed = [this](const std::string& text) {
        m_text = text;
    };
    m_dialog->callbacks().font_selection_changed =
        [this](const Domain::FontDescriptor& font_descriptor) {
            m_preset_manager.get_preset().emboss_style.descriptor = font_descriptor;
        };    
    m_dialog->callbacks().style_selection_changed = [this](int id) {

        // TODO: implement
        };

    m_dialog->callbacks().save_preset_as = [this]() { m_preset_manager.store_presets(); };
    m_dialog->callbacks().save_preset = [this]() { m_preset_manager.store_presets(); };
    m_dialog->callbacks().rename_preset = [this]() { m_preset_manager.store_presets(); };
    m_dialog->callbacks().delete_preset = [this]() { 
        std::string style_name = m_preset_manager.get_preset().emboss_style.descriptor.name; // copy
        size_t next_style_index = std::numeric_limits<size_t>::max();
        bool exist_change = false;
        while (true) {
            // NOTE: can't use previous loaded activ index -> erase could change index
            size_t active_index = m_preset_manager.get_preset_index();
            next_style_index = (active_index > 0) ? active_index - 1 :
                active_index + 1;

            if (next_style_index >= m_preset_manager.get_styles().size()) {
                //MessageDialog msg(plater, _L("Can't remove the last existing preset."), dialog_title, wxICON_ERROR | wxOK);
                //msg.ShowModal();
                break;
            }

            // IMPROVE: add function can_load?
            // clean unactivable styles
            if (!m_preset_manager.load_preset(next_style_index)) {
                m_preset_manager.erase(next_style_index);
                exist_change = true;
                continue;
            }

            //wxString message = GUI::format_wxstr(_L("Are you sure you want to permanently remove the \"%1%\" preset?"), style_name);
            //MessageDialog msg(plater, message, dialog_title, wxICON_WARNING | wxYES | wxNO);
            //if (msg.ShowModal() == wxID_YES) {
                // delete preset
                m_preset_manager.erase(active_index);
                exist_change = true;
                //process();
            //}
            //else {
            //    // load back preset
            //    m_preset_manager.load_preset(active_index);
            //}
            break;
        }
        if (exist_change) {
            m_preset_manager.store_presets(false);
            activate_preset();
        }
    };
    m_dialog->callbacks().text_changed = [this](const std::string& text) {
        m_text = text;
    };
    m_dialog->callbacks().set_on_face_camera = [this]() {
        m_dialog->show_revert_buttons(false); // test
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
        Domain::TextConfiguration text_config{
            .style = m_preset_manager.get_preset().emboss_style,
            .text = m_text
        };
        auto params = create_volume_params(
            m_project_interactor,
            m_font_manager,
            std::move(text_config),
            m_projection,
            Domain::ModelVolumeType::NEGATIVE_VOLUME
        );
        if (Biz::Emboss::start_create_volume(params, ctx.pick_ray(), ctx.pick_results()))
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
    if (ImGui::Begin("Text Gizmo")) {
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
            const auto& styles = m_preset_manager.get_styles();
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

        ImGui::Text("Emboss text");
        if (ImGui::Button("Close")) {
            close();
        }
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
}

void TextGizmo::on_activated()
{
    if (m_preset_manager.get_styles().empty())
        m_preset_manager.init();
    m_text = "Emmmbosss text";

    m_dialog->set_presets(m_preset_manager.get_style_names(), m_preset_manager.get_preset_index());

    // load current font_preset
    activate_preset(/*font_preset*/);

    bool use_inch = false; // wxGetApp().app_config->get_bool("use_inches");
    m_dialog->update_units(use_inch);

    // unknown font, so only font selection is enabled
    m_dialog->set_enable_all_except_font(true);

    // Propadate reloaded installed font into the dialog
    // NOTE: reload fonts from OS, 2.9.2 do it on dialog open, now it is on gizmo activation
    const Domain::FontList& fonts = m_font_manager.get_fonts(); // Re-Load Os fonts
    const Domain::EmbossStyle& es = m_preset_manager.get_preset().emboss_style;
    const Domain::EmbossStyle& es_ = m_preset_manager.exist_stored_style()?
        m_preset_manager.get_stored_preset()->emboss_style : es;
    int selected_font_id = (es.descriptor.type == m_font_manager.get_current_type()) ?
        get_index(fonts, es.descriptor.path) : 0;
    int default_font_id = get_index(fonts, es_.descriptor.path);
    m_dialog->set_fonts(fonts, selected_font_id, default_font_id);
}

void TextGizmo::on_deactivated() {}

Scene::ToolType TextGizmo::type() const {
    return Scene::ToolType::Text;
}

bool TextGizmo::add_text_by_view_direction(Domain::ModelVolumeType volume_type)
{
    if (m_gizmo_manager.current_tool_type() == type())
        return false; // already active

    if (!init_create(volume_type))
        return false;

    Domain::TextConfiguration text_config{
        .style = m_preset_manager.get_preset().emboss_style,
        .text  = m_text
    };
    auto params = create_volume_params(
        m_project_interactor,
        m_font_manager,
        std::move(text_config),
        m_projection,
        volume_type
    );
    return Biz::Emboss::start_create_volume_without_position(params);
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

void TextGizmo::update_presets_list() {}

void TextGizmo::activate_preset(/*preset*/)
{    
    bool exist_stored = m_preset_manager.exist_stored_style();
    const Biz::Emboss::TextPresetManager::Preset& preset = m_preset_manager.get_preset();
    const Biz::Emboss::TextPresetManager::Preset& preset_ = exist_stored ?
        *m_preset_manager.get_stored_preset() : preset;

    const Domain::EmbossStyle& es = preset.emboss_style;
    const Domain::EmbossStyle& es_ = preset_.emboss_style;

    // TODO: solve conversion from font name
    std::vector<std::string> styles = { "Regular", "Bold", "Italic", "ItalicBold" };
    int selected_style_id = 0;
    int default_style_id = 0;
    m_dialog->set_styles(styles, selected_style_id, default_style_id);    
    m_dialog->set_editor(m_text);

    const Domain::FontProp& prop = es.prop;
    const Domain::FontProp& prop_ = es_.prop;
    double height_from = 0.1;
    double height_to = 100.;
    double height_step = 0.1;
    double height_step_fast = 1;
    double height = prop.size_in_mm;
    double height_default = prop_.size_in_mm;
    m_dialog->set_height(height_from, height_to, height_step, height_step_fast, height, height_default);

    const Domain::EmbossProjection& ep = m_projection;
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
    bool allowe_surface_distance = !m_projection.use_surface;// && !m_volume->is_the_only_one_part();
    m_dialog->set_enable_surface_distance(allowe_surface_distance);

    double rotation_max = 180.;
    double rotation_step = 0.1;
    double rotation = 92.;
    double rotation_ = preset_.angle.value_or(0.f);
    m_dialog->set_rotation(rotation_max, rotation_step, rotation, rotation_);    
}

} // namespace Slic3r::App::Plater
