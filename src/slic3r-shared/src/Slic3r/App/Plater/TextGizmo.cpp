///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextGizmo.hpp"
#include "Slic3r/App/Plater/TextDialog.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

TextGizmo::TextGizmo()
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

    m_dialog->callbacks().preset_selection_changed = [](int id) {
    };
    m_dialog->callbacks().font_selection_changed = [](int id) {
    };
    m_dialog->callbacks().style_selection_changed = [](int id) {
    };
    m_dialog->callbacks().operation_selection_changed = [](int id) {
    };
}

void TextGizmo::on_activated()
{
    std::vector<std::string> presets = {"NORMAL", "SMALL", "ITALIC", "SWISS"};
    int selected_preset_id           = 2;
    m_dialog->set_presets(presets, selected_preset_id);

    // load current font_preset
    activate_preset(/*font_preset*/);

    bool use_inch = true; // wxGetApp().app_config->get_bool("use_inches");
    m_dialog->update_units(use_inch);
    m_dialog->set_enable_all_except_font(true); // test
}

void TextGizmo::on_deactivated() {}

Scene::ToolType TextGizmo::type() const
{
    return Scene::ToolType::TextGizmo;
}

Yoga::GizmoWindowPtr TextGizmo::release_ui_window()
{
    return m_dialog.release();
}

Scene::GizmoActivationState TextGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
}

void TextGizmo::update_layout(bool show_for_part)
{
    m_dialog->show_part_specific_panel(show_for_part);
}

void TextGizmo::update_presets_list() {}

void TextGizmo::activate_preset(/*preset*/)
{
    // Propadate data to the dialog

    std::vector<std::string> fonts = {"Arial", "Calibri", "Cambria"};
    int selected_font_id           = 1;
    int default_font_id            = 2;
    m_dialog->set_fonts(fonts, selected_font_id, default_font_id);

    std::vector<std::string> styles = {"Regular", "Bold", "Italic", "ItalicBold"};
    int selected_style_id           = 0;
    int default_style_id            = 0;
    m_dialog->set_styles(styles, selected_style_id, default_style_id);

    double height_from      = 0.1;
    double height_to        = 100.;
    double height_step      = 0.1;
    double height_step_fast = 1;
    double height           = 10.;
    double height_default   = 8.;
    m_dialog->set_height(height_from, height_to, height_step, height_step_fast, height, height_default);

    double depth_from      = 0.1;
    double depth_to        = 100.;
    double depth_step      = 0.1;
    double depth_step_fast = 1;
    double depth           = 8.;
    double depth_default   = 8.;
    m_dialog->set_depth(depth_from, depth_to, depth_step, depth_step_fast, depth, depth_default);

    bool use_surface         = true;
    bool use_surface_default = false;
    m_dialog->set_use_surface(use_surface, use_surface_default);

    bool per_glyph         = true;
    bool per_glyph_default = false;
    m_dialog->set_per_glyph(per_glyph, per_glyph_default);

    Domain::TextAlign align = {Domain::HorizontalAlign::left, Domain::VerticalAlign::bottom};
    m_dialog->set_align(align);

    double char_gap_max  = 3.62;
    double char_gap_step = 0.01;
    double char_gap      = 0.25;
    m_dialog->set_char_gap(char_gap_max, char_gap_step, char_gap);

    double line_gap_max  = 3.62;
    double line_gap_step = 0.01;
    double line_gap      = 0.25;
    m_dialog->set_line_gap(line_gap_max, line_gap_step, line_gap);

    double boldness_max  = 0.8;
    double boldness_step = 0.1;
    double boldness      = 0.34;
    m_dialog->set_boldness(boldness_max, boldness_step, boldness);

    double skew_ratio_max  = 1.;
    double skew_ratio_step = 0.01;
    double skew_ratio      = -0.72;
    m_dialog->set_skew_ratio(skew_ratio_max, skew_ratio_step, skew_ratio);

    double surface_distance_max  = 2.;
    double surface_distance_step = 0.01;
    double surface_distance      = 0.;
    m_dialog->set_surface_distance(surface_distance_max, surface_distance_step, surface_distance, 0.);

    double rotation_max  = 180.;
    double rotation_step = 0.1;
    double rotation      = 92.;
    m_dialog->set_rotation(rotation_max, rotation_step, rotation);
}

} // namespace Slic3r::App::Plater
