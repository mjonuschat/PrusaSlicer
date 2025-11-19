///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/CutDialog.hpp"

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/Biz/Expr/Parser.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"
#include "Slic3r/Math.hpp"

#include <fmt/format.h>

using namespace Slic3r::Biz;

namespace Slic3r::App::Yoga {

static const float label_width_percent{25.f};

static LayoutButton* add_button(Item* parent, Render::Icon icon, const std::string& tooltip)
{
    LayoutButton* btn = parent->emplace_back<LayoutButton>("", icon, tooltip);
    btn->set_checkable(true);
    btn->set_min_size(Vec2f(40.f, 40.f));
    return btn;
}

Slic3r::App::Yoga::PartProcessingRow::PartProcessingRow(const std::string& part_name) :
    m_name(part_name)
{
    Item* group_item = this->emplace_back<Item>();
    group_item->set_gap(10.f);
    m_part_checker = group_item->emplace_back<ToggleButton>(m_name);
    m_part_checker->set_width(100);
    m_part_checker->set_checked(true);
    m_part_checker->callbacks().checked_changed = [this](bool checked)
    {
        set_enabled_buttons(checked);
        if (callbacks().checked_changed) {
            callbacks().checked_changed(checked);
        }
    };

    m_keep_btn         = add_button(group_item, Render::Icon::Pin, _u8L("Keep orientation"));
    m_place_on_cut_btn = add_button(group_item, Render::Icon::PlaceOnFace, _u8L("Place on cut"));
    m_flip_btn = add_button(group_item, Render::Icon::FlipVertically, _u8L("Flip upside down"));

    m_group.set_buttons({m_keep_btn, m_place_on_cut_btn, m_flip_btn});
    m_group.callbacks().action = [this](AbstractButton* btn)
    {
        Action action = btn == m_keep_btn ? Action::Keep :
            btn == m_place_on_cut_btn     ? Action::PlaceOnCut :
                                            Action::Flip;
        if (callbacks().part_action_changed) {
            callbacks().part_action_changed(action);
        }
    };
}

PartProcessingRow::~PartProcessingRow() {}

PartProcessingRow::Callbacks& PartProcessingRow::callbacks()
{
    return m_callbacks;
}

void PartProcessingRow::set_as_part(bool is_part)
{
    const std::string new_name =
        fmt::format("{} {}", is_part ? _u8L("Part") : _u8L("Object"), m_name);
    m_part_checker->set_label(new_name);

    if (is_part) {
        m_part_checker->set_checked(true);
        m_keep_btn->set_checked(true);
    }

    set_enabled(!is_part);
    set_enabled_buttons(!is_part);
}

void PartProcessingRow::set_enabled_buttons(bool enabled)
{
    for (AbstractButton* btn : m_group.buttons()) {
        btn->set_enabled(enabled);
    }
}

bool PartProcessingRow::is_checked() const
{
    return m_part_checker->checked();
}

static LayoutButton* add_revert_btn(Item* parent, const std::string& tooltip)
{
    Item* revert_space = parent->emplace_back<Item>();
    revert_space->set_width_percent(10);
    revert_space->set_justify_content(YGJustifyFlexEnd);
    LayoutButton* revert_btn =
        revert_space->emplace_back<LayoutButton>("", Render::Icon::DSRevert, tooltip);
    revert_btn->set_self_align(YGAlignCenter);
    revert_btn->set_aspect_ratio(1);
    // revert_btn->set_visible(false);
    return revert_btn;
}

} // namespace Slic3r::App::Yoga

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

CutDialog::Callbacks& CutDialog::callbacks()
{
    return m_callbacks;
}

static const Vec2f mouse_help_size{25.f, 25.f};
static const Vec2f shortcut_help_size{40.f, 25.f};

static const ImColor build_volume_color{151, 187, 255};
static const ImColor titles_color{121, 149, 203};
static const ImColor buttons_color{76, 93, 127};

CutDialog::CutDialog() : GizmoDialog(_u8L("Cut"))
{
    content_item()->set_width(400);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size());

    init_connectors_input_panel();
    init_cut_plane_input_panel();

    m_warnings_line = content()->emplace_back<Text>("There will be warning");

    update_state();
}

void CutDialog::init_cut_plane_input_panel()
{
    m_cut_plane_input_panel = content()->emplace_back<Item>();
    m_cut_plane_input_panel->set_orientation(Orientation::Vertical);
    m_cut_plane_input_panel->set_align_content(YGAlign::YGAlignFlexStart);
    m_cut_plane_input_panel->set_gap(gap_size());

    Item* mode_row    = add_row(_u8L("Mode"), m_cut_plane_input_panel);
    m_planar_mode_btn = add_button(mode_row, Render::Icon::DividingLine, _u8L("Planar"));
    LayoutButton* dovetail_mode_btn = add_button(mode_row, Render::Icon::Dove, _u8L("Dovetail"));
    m_mode_group.set_buttons({m_planar_mode_btn, dovetail_mode_btn});
    m_mode_group.callbacks().action = [this](AbstractButton* btn)
    {
        is_planar_cut_mode = btn == m_planar_mode_btn;
        update_panels_visibility();
        if (callbacks().mode_changed) {
            callbacks().mode_changed();
        }

        if (!is_planar_cut_mode) {
            keep_as_parts = false;
            m_cut_into_objects_btn->set_checked(true);
            m_part_A->set_as_part(keep_as_parts);
            m_part_B->set_as_part(keep_as_parts);
        }

        // disable buttons for dovetail mode
        m_cut_into_objects_btn->set_enabled(is_planar_cut_mode);
        m_cut_into_parts_btn->set_enabled(is_planar_cut_mode);
        m_add_connectors_btn->set_visible(is_planar_cut_mode);
    };

    Item* buid_volume_row = add_row(_u8L("Build Volume"), m_cut_plane_input_panel);
    m_build_volume        = buid_volume_row->emplace_back<Text>("X mm, Y mm, Z mm");
    m_build_volume->set_text_color(build_volume_color);
    m_build_volume->set_font_type(Render::ImguiFontType::Bold);

    Item* cut_position_row = add_row(_u8L("Cut position"), m_cut_plane_input_panel);
    cut_position_row->emplace_back<Text>("Z:");
    m_cut_position = cut_position_row->emplace_back<InputTextField>();
    m_cut_position->set_width(50);
    std::unique_ptr<DoubleValidator> validator = std::make_unique<DoubleValidator>();
    validator->set_precision(2);
    m_cut_position->set_validator(std::move(validator));
    m_cut_position->callbacks().text_changed = [this]()
    {
        if (callbacks().z_changed) {
            Slic3r::Biz::Expr::Parser parser;
            Slic3r::Biz::Expr::Eval eval;
            try {
                double new_z = boost::get<double>(eval.eval(parser.parse(m_cut_position->text())));
                callbacks().z_changed(new_z);
            } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
            } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
            }
        }
    };

    LayoutButton* revert_cut_position_btn =
        add_revert_btn(cut_position_row, _u8L("Reset cutting plane"));
    m_cut_position->set_revert_button(revert_cut_position_btn);

    Item* buttons_row = m_cut_plane_input_panel->emplace_back<Item>();
    buttons_row->set_justify_content(YGJustifySpaceBetween);

    m_add_connectors_btn = buttons_row->emplace_back<LayoutButton>(
        /*has_connectors ? _u8L("Edit connectors") : */ _u8L("Add connectors")
    );
    m_add_connectors_btn->set_background_color(buttons_color);
    m_add_connectors_btn->callbacks().action = [this]()
    {
        connectors_editing = true;
        if (callbacks().connectors_editing_changed) {
            callbacks().connectors_editing_changed(connectors_editing);
        }
        update_panels_visibility();
    };

    // Add empty item for correct layout, when "Add connectors" button is hidden
    buttons_row->emplace_back<Item>();

    LayoutButton* reset_cut_btn = buttons_row->emplace_back<LayoutButton>(
        _u8L("Reset cut"),
        Render::Icon::None,
        _u8L("Reset cutting plane and remove connectors")
    );
    reset_cut_btn->set_background_color(buttons_color);
    reset_cut_btn->callbacks().action = [this]()
    {
        if (callbacks().reset_cut_plane) {
            callbacks().reset_cut_plane();
        }
    };

    add_groove_input_panel();
    add_cut_settings();
    add_cut_plane_help_panel();

    m_perform_btn = m_cut_plane_input_panel->emplace_back<LayoutButton>(_u8L("Perform cut"));
    m_perform_btn->set_background_color(buttons_color);
    m_perform_btn->callbacks().action = [this]()
    {
        if (callbacks().perform) {
            callbacks().perform();
        }
    };
}

void CutDialog::add_cut_plane_help_panel()
{
    add_separator(m_cut_plane_input_panel);
    Item* help_row = m_cut_plane_input_panel->emplace_back<Item>();

    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_padding(5);

    Yoga::GizmoDialogHelp help;
    help.init(help_row);
    help.add_item({{Render::Icon::KeyShift, shortcut_help_size}}, _u8L("Hold to draw a cut line"));
    add_separator(m_cut_plane_input_panel);
}

void CutDialog::add_groove_input_panel()
{
    m_groove_input_panel = m_cut_plane_input_panel->emplace_back<Item>();
    m_groove_input_panel->set_orientation(Orientation::Vertical);
    m_groove_input_panel->set_align_content(YGAlign::YGAlignFlexStart);
    m_groove_input_panel->set_gap(gap_size());

    add_separator(m_groove_input_panel);

    Text* text = m_groove_input_panel->emplace_back<Text>(_u8L("Groove") + ":");
    text->set_text_color(titles_color);
    text->set_font_type(Render::ImguiFontType::Bold);

    Item* depth_row                = add_row(_u8L("Depth"), m_groove_input_panel, "mm");
    LayoutButton* revert_depth_btn = add_revert_btn(depth_row, _u8L("Revert depth"));

    m_groove_depth_value                            = depth_row->emplace_back<SliderWithInput>();
    m_groove_depth_value->callbacks().value_changed = [this](double value)
    {
        if (callbacks().groove_depth_value_changed) {
            callbacks().groove_depth_value_changed(value);
        }
    };

    m_groove_depth_tolerance = depth_row->emplace_back<SliderWithInput>();
    m_groove_depth_tolerance->callbacks().value_changed = [this](double value)
    {
        if (callbacks().groove_depth_tolerance_changed) {
            callbacks().groove_depth_tolerance_changed(value);
        }
    };

    m_groove_depth_value->set_revert_button(revert_depth_btn);
    m_groove_depth_tolerance->set_revert_button(revert_depth_btn);

    Item* width_row                = add_row(_u8L("Width"), m_groove_input_panel, "mm");
    LayoutButton* revert_width_btn = add_revert_btn(width_row, _u8L("Revert width"));

    m_groove_width_value                            = width_row->emplace_back<SliderWithInput>();
    m_groove_width_value->callbacks().value_changed = [this](double value)
    {
        if (callbacks().groove_width_value_changed) {
            callbacks().groove_width_value_changed(value);
        }
    };

    m_groove_width_tolerance = width_row->emplace_back<SliderWithInput>();
    m_groove_width_tolerance->callbacks().value_changed = [this](double value)
    {
        if (callbacks().groove_width_tolerance_changed) {
            callbacks().groove_width_tolerance_changed(value);
        }
    };

    m_groove_width_value->set_revert_button(revert_width_btn);
    m_groove_width_tolerance->set_revert_button(revert_width_btn);

    Item* flap_angle_row = add_row(_u8L("Flap Angle"), m_groove_input_panel, std::string("°"));
    LayoutButton* revert_flap_angle_btn = add_revert_btn(flap_angle_row, _u8L("Revert flap angle"));

    m_flap_angle                            = flap_angle_row->emplace_back<SliderWithInput>();
    m_flap_angle->callbacks().value_changed = [this](double value)
    {
        if (callbacks().flap_angle_changed) {
            callbacks().flap_angle_changed(value);
        }
    };
    m_flap_angle->set_revert_button(revert_flap_angle_btn);

    Item* groove_angle_row = add_row(_u8L("Groove Angle"), m_groove_input_panel, std::string("°"));
    LayoutButton* revert_groove_angle_btn =
        add_revert_btn(groove_angle_row, _u8L("Revert groove angle"));

    m_groove_angle                            = groove_angle_row->emplace_back<SliderWithInput>();
    m_groove_angle->callbacks().value_changed = [this](double value)
    {
        if (callbacks().groove_angle_changed) {
            callbacks().groove_angle_changed(value);
        }
    };
    m_groove_angle->set_revert_button(revert_groove_angle_btn);

    for (SliderWithInput* input :
         {m_groove_depth_value,
          m_groove_depth_tolerance,
          m_groove_width_value,
          m_groove_width_tolerance,
          m_flap_angle,
          m_groove_angle})
    {
        input->set_input_width(40);
    }

    for (SliderWithInput* input : {m_groove_width_tolerance, m_groove_depth_tolerance}) {
        input->set_flex_grow(1);
    }

    for (SliderWithInput* input :
         {m_groove_depth_value, m_groove_width_value, m_flap_angle, m_groove_angle})
    {
        input->set_width(120);
    }

    m_groove_input_panel->set_visible(false);
}

void CutDialog::add_cut_settings()
{
    add_separator(m_cut_plane_input_panel);

    Item* cut_into_row = add_row(_u8L("Cut into") + ":", m_cut_plane_input_panel);

    // TRN CutGizmo: RadioButton Cut into ...
    m_cut_into_objects_btn = add_button(cut_into_row, Render::Icon::ObjectIcon, _u8L("Objects"));
    // TRN CutGizmo: RadioButton Cut into ...
    m_cut_into_parts_btn = add_button(cut_into_row, Render::Icon::SolidPartVolume, _u8L("Parts"));

    m_cut_into_group.set_buttons({m_cut_into_objects_btn, m_cut_into_parts_btn});
    m_cut_into_group.callbacks().action = [this](AbstractButton* btn)
    {
        keep_as_parts = btn == m_cut_into_parts_btn;
        m_part_A->set_as_part(keep_as_parts);
        m_part_B->set_as_part(keep_as_parts);

        m_add_connectors_btn->set_visible(btn == m_cut_into_objects_btn);
    };

    // Text* text = m_cut_plane_input_panel->emplace_back<Text>(_u8L("Cut result") + ":");
    Item* cut_result_row = add_row(_u8L("Cut result") + ":", m_cut_plane_input_panel);

    Item* parts_wrap_item = cut_result_row->emplace_back<Item>();
    parts_wrap_item->set_orientation(Orientation::Vertical);
    parts_wrap_item->set_gap(gap_size());

    // create PartProcessingRow

    m_part_A = parts_wrap_item->emplace_back<PartProcessingRow>("A");
    m_part_B = parts_wrap_item->emplace_back<PartProcessingRow>("B");

    m_part_A->set_as_part(false);
    m_part_B->set_as_part(false);

    m_part_A->callbacks().checked_changed     = [this](bool checked) { keep_upper = checked; };
    m_part_A->callbacks().part_action_changed = [this](PartProcessingRow::Action action)
    {
        keep_upper         = m_part_A->is_checked() || action == PartProcessingRow::Action::Keep;
        place_on_cut_upper = action == PartProcessingRow::Action::PlaceOnCut;
        flip_upper         = action == PartProcessingRow::Action::Flip;
    };

    m_part_B->callbacks().checked_changed     = [this](bool checked) { keep_lower = checked; };
    m_part_B->callbacks().part_action_changed = [this](PartProcessingRow::Action action)
    {
        keep_lower         = m_part_B->is_checked() || action == PartProcessingRow::Action::Keep;
        place_on_cut_lower = action == PartProcessingRow::Action::PlaceOnCut;
        flip_lower         = action == PartProcessingRow::Action::Flip;
    };
}

void CutDialog::update_state()
{
    bool has_warning{false};
    // check warning statet here

    m_warnings_line->set_visible(has_warning);
    m_perform_btn->set_visible(!has_warning);
}

void CutDialog::update_panels_visibility()
{
    m_connectors_input_panel->set_visible(connectors_editing);
    m_cut_plane_input_panel->set_visible(!connectors_editing);
    m_groove_input_panel->set_visible(!is_planar_cut_mode);
}

void CutDialog::init_connectors_input_panel()
{
    m_connectors_input_panel = content()->emplace_back<Item>();
    m_connectors_input_panel->set_orientation(Orientation::Vertical);
    m_connectors_input_panel->set_align_content(YGAlign::YGAlignFlexStart);
    m_connectors_input_panel->set_gap(gap_size());

    Item* title_row = m_connectors_input_panel->emplace_back<Item>();
    title_row->set_gap(gap_size());
    title_row->set_align_items(YGAlignCenter);

    Text* text = title_row->emplace_back<Text>(_u8L("Connectors") + ":");
    text->set_text_color(titles_color);
    text->set_font_type(Render::ImguiFontType::Bold);

    LayoutButton* reset_btn       = add_revert_btn(title_row, _u8L("Remove connectors"));
    reset_btn->callbacks().action = [this]()
    {
        if (callbacks().reset_connectors) {
            callbacks().reset_connectors();
        }
    };

    LayoutButton* flip_cut_plane_btn =
        title_row->emplace_back<LayoutButton>(_u8L("Flip cut plane"));
    flip_cut_plane_btn->set_background_color(buttons_color);
    flip_cut_plane_btn->callbacks().action = [this]()
    {
        if (callbacks().flip_cut_plane) {
            callbacks().flip_cut_plane();
        }
    };

    m_type_row = add_row(_u8L("Type"), m_connectors_input_panel);

    m_plug_btn  = add_button(m_type_row, Render::Icon::PlugMarker, _u8L("Plug"));
    m_dowel_btn = add_button(m_type_row, Render::Icon::DowelMarker, _u8L("Dowel"));
    m_snap_btn  = add_button(m_type_row, Render::Icon::SnapMarker, _u8L("Snap"));

    m_connector_type_group.set_buttons({m_plug_btn, m_dowel_btn, m_snap_btn});
    m_connector_type_group.set_always_checked(false);
    m_connector_type_group.callbacks().action = [this](AbstractButton* btn)
    {
        const Domain::CutConnectorType type = btn == m_plug_btn ? Domain::CutConnectorType::Plug :
            btn == m_dowel_btn                                  ? Domain::CutConnectorType::Dowel :
                                                                  Domain::CutConnectorType::Snap;
        set_connector_type(type);

        if (callbacks().connector_attributes_changed) {
            callbacks().connector_attributes_changed();
        }
    };

    m_style_row   = add_row(_u8L("Style"), m_connectors_input_panel);
    m_prism_btn   = add_button(m_style_row, Render::Icon::Prism, _u8L("Prism"));
    m_frustum_btn = add_button(m_style_row, Render::Icon::Frustum, _u8L("Frustum"));
    m_connector_style_group.set_buttons({m_prism_btn, m_frustum_btn});
    m_connector_style_group.set_always_checked(false);
    m_connector_style_group.callbacks().action = [this](AbstractButton* btn)
    {
        set_connector_style(
            btn == m_prism_btn ? Domain::CutConnectorStyle::Prism :
                                 Domain::CutConnectorStyle::Frustum
        );
        if (callbacks().connector_attributes_changed) {
            callbacks().connector_attributes_changed();
        }
    };

    m_shape_row    = add_row(_u8L("Shape"), m_connectors_input_panel);
    m_triangle_btn = add_button(m_shape_row, Render::Icon::Triangle, _u8L("Triangle"));
    m_square_btn   = add_button(m_shape_row, Render::Icon::Square, _u8L("Square"));
    m_hexagon_btn  = add_button(m_shape_row, Render::Icon::Hexagon, _u8L("Hexagon"));
    m_circle_btn   = add_button(m_shape_row, Render::Icon::Circle, _u8L("Circle"));
    m_connector_shape_group.set_buttons(
        {m_triangle_btn, m_square_btn, m_hexagon_btn, m_circle_btn}
    );
    m_connector_shape_group.set_always_checked(false);
    m_connector_shape_group.callbacks().action = [this](AbstractButton* btn)
    {
        set_connector_shape(
            btn == m_triangle_btn   ? Domain::CutConnectorShape::Triangle :
                btn == m_square_btn ? Domain::CutConnectorShape::Square :
                btn == m_circle_btn ? Domain::CutConnectorShape::Circle :
                                      Domain::CutConnectorShape::Hexagon
        );
        if (callbacks().connector_attributes_changed) {
            callbacks().connector_attributes_changed();
        }
    };

    Item* depth_row                = add_row(_u8L("Depth"), m_connectors_input_panel, "mm");
    LayoutButton* revert_depth_btn = add_revert_btn(depth_row, _u8L("Revert depth"));

    m_connector_depth_value                            = depth_row->emplace_back<SliderWithInput>();
    m_connector_depth_value->callbacks().value_changed = [this](double value)
    {
        connector_depth = value;
        if (callbacks().connector_transformations_changed)
            callbacks().connector_transformations_changed();
    };
    m_connector_depth_tolerance = depth_row->emplace_back<SliderWithInput>();
    m_connector_depth_tolerance->callbacks().value_changed = [this](double value)
    {
        connector_depth_tolerance = value;
        if (callbacks().connector_transformations_changed)
            callbacks().connector_transformations_changed();
    };
    m_connector_depth_value->set_revert_button(revert_depth_btn);
    m_connector_depth_tolerance->set_revert_button(revert_depth_btn);

    Item* size_row                = add_row(_u8L("Size"), m_connectors_input_panel, "mm");
    LayoutButton* revert_size_btn = add_revert_btn(size_row, _u8L("Revert width"));

    m_connector_size_value                            = size_row->emplace_back<SliderWithInput>();
    m_connector_size_value->callbacks().value_changed = [this](double value)
    {
        connector_size = value;
        if (callbacks().connector_transformations_changed)
            callbacks().connector_transformations_changed();
    };
    m_connector_size_tolerance = size_row->emplace_back<SliderWithInput>();
    m_connector_size_tolerance->callbacks().value_changed = [this](double value)
    {
        connector_size_tolerance = value;
        if (callbacks().connector_transformations_changed)
            callbacks().connector_transformations_changed();
    };
    m_connector_size_value->set_revert_button(revert_size_btn);
    m_connector_size_tolerance->set_revert_button(revert_size_btn);

    m_rotation_row = add_row(_u8L("Rotation"), m_connectors_input_panel, std::string("°"));
    LayoutButton* revert_rotation_btn =
        add_revert_btn(m_rotation_row, _u8L("Revert connector Z rotation"));

    m_connector_rotation = m_rotation_row->emplace_back<SliderWithInput>();
    m_connector_rotation->callbacks().value_changed = [this](double value)
    {
        connector_angle = value;
        if (callbacks().connector_transformations_changed)
            callbacks().connector_transformations_changed();
    };
    m_connector_rotation->set_revert_button(revert_rotation_btn);

    m_snap_bulge_row = add_row(_u8L("Bulge"), m_connectors_input_panel, std::string("%"));
    LayoutButton* revert_snap_bulge_btn =
        add_revert_btn(m_snap_bulge_row, _u8L("Revert bulge proportion related to radius"));

    m_snap_bulge_proportion = m_snap_bulge_row->emplace_back<SliderWithInput>();
    m_snap_bulge_proportion->callbacks().value_changed = [this](double value)
    {
        snap_bulge_proportion = value;
        if (callbacks().snap_settings_changed)
            callbacks().snap_settings_changed();
    };
    m_snap_bulge_proportion->set_revert_button(revert_snap_bulge_btn);

    m_snap_space_row = add_row(_u8L("Space"), m_connectors_input_panel, std::string("%"));
    LayoutButton* revert_snap_space_btn =
        add_revert_btn(m_snap_space_row, _u8L("Revert space proportion related to radius"));

    m_snap_space_proportion = m_snap_space_row->emplace_back<SliderWithInput>();
    m_snap_space_proportion->callbacks().value_changed = [this](double value)
    {
        snap_space_proportion = value;
        m_snap_bulge_proportion->set_end_value(snap_space_proportion);
        if (callbacks().snap_settings_changed)
            callbacks().snap_settings_changed();
    };
    m_snap_space_proportion->set_revert_button(revert_snap_space_btn);

    for (SliderWithInput* input : {
             m_connector_depth_value,
             m_connector_depth_tolerance,
             m_connector_size_value,
             m_connector_size_tolerance,
             m_connector_rotation,
             m_snap_bulge_proportion,
             m_snap_space_proportion,
         })
    {
        input->set_input_width(40);
    }

    for (SliderWithInput* input : {m_connector_depth_tolerance, m_connector_size_tolerance}) {
        input->set_flex_grow(1);
    }

    for (SliderWithInput* input :
         {m_connector_depth_value,
          m_connector_size_value,
          m_connector_rotation,
          m_snap_bulge_proportion,
          m_snap_space_proportion})
    {
        input->set_width(120);
    }

    add_separator(m_connectors_input_panel);

    Item* buttons_row = m_connectors_input_panel->emplace_back<Item>();
    buttons_row->set_justify_content(YGJustifySpaceBetween);

    LayoutButton* confirm_connectors_btn =
        buttons_row->emplace_back<LayoutButton>(_u8L("Confirm connectors"));
    confirm_connectors_btn->set_background_color(buttons_color);
    confirm_connectors_btn->callbacks().action = [this]()
    {
        connectors_editing = false;
        if (callbacks().connectors_editing_changed) {
            callbacks().connectors_editing_changed(connectors_editing);
        }
        update_panels_visibility();
    };

    LayoutButton* cancel_btn = buttons_row->emplace_back<LayoutButton>(_u8L("Cancel"));
    cancel_btn->set_background_color(buttons_color);
    cancel_btn->callbacks().action = [this]()
    {
        connectors_editing = false;
        if (callbacks().connectors_editing_changed) {
            callbacks().connectors_editing_changed(connectors_editing);
        }
        update_panels_visibility();
        if (callbacks().reset_connectors) {
            callbacks().reset_connectors();
        }
    };

    add_connectors_help_panel();

    m_connectors_input_panel->set_visible(false);
}

void CutDialog::add_connectors_help_panel()
{
    ImColor color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    add_separator(m_connectors_input_panel);
    m_connectors_input_panel->emplace_back<Text>(_u8L("Connectors") + ":")->set_text_color(color);

    Item* help_row = m_connectors_input_panel->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_gap(10);

    Yoga::GizmoDialogHelp help;
    help.init(help_row);
    help.add_item({{Render::Icon::MouseLeft, mouse_help_size}}, _u8L("Add"));
    help.add_item({{Render::Icon::MouseRight, mouse_help_size}}, _u8L("Remove"));
    help.add_item({{Render::Icon::MouseDrag, mouse_help_size}}, _u8L("Move"));

    add_separator(m_connectors_input_panel);
    m_connectors_input_panel->emplace_back<Text>(_u8L("Selection") + ":")->set_text_color(color);

    Item* help_row2 = m_connectors_input_panel->emplace_back<Item>();
    help_row2->set_min_size({0, 50});
    help_row2->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row2->set_gap(10);

    Yoga::GizmoDialogHelp help2;
    help2.init(help_row2);
    help2.add_item(
        {{Render::Icon::KeyShift, shortcut_help_size}, {Render::Icon::MouseLeft, mouse_help_size}},
        _u8L("Add")
    );
    help2.add_item(
        {{Render::Icon::KeyAlt, mouse_help_size}, {Render::Icon::MouseLeft, mouse_help_size}},
        _u8L("Remove")
    );
    help2.add_item({{Render::Icon::KeyCtrlA, shortcut_help_size}}, _u8L("Select all"));
}

Item* CutDialog::add_row(const std::string& title, Yoga::Item* parent, const std::string& unit)
{
    Item* row = parent->emplace_back<Item>();
    row->set_gap(3);

    Text* text = row->emplace_back<Text>(title);
    text->set_width_percent(label_width_percent);
    text->set_self_align(YGAlignCenter);

    Item* box = row->emplace_back<Item>();
    box->set_width_percent(70);
    box->set_gap(gap_size());
    box->set_align_items(YGAlignCenter);

    if (!unit.empty()) {
        Text* unit_text = row->emplace_back<Text>(unit);
        if (unit == "mm") {
            // Means that unit will be changed in respect to the "use_inches" app_config option
            // so, add it to the m_units vector
            m_units.emplace_back(unit_text);
        }
        unit_text->set_self_align(YGAlignCenter);
    }

    return box;
}

void CutDialog::set_build_size(Domain::Vec3d size)
{
    static const double in_to_mm = 25.4;
    static const double mm_to_in = 1 / in_to_mm;

    std::string unit_str = m_imperial_units ? _u8L("in") : _u8L("mm");

    if (m_imperial_units) {
        size.x() *= mm_to_in;
        size.y() *= mm_to_in;
        size.z() *= mm_to_in;
    }

    m_build_volume->set_text(
        fmt::format("{0:.5g} {3}, {1:.5g} {3}, {2:.5g} {3}", size.x(), size.y(), size.z(), unit_str)
    );
}

void CutDialog::set_cut_z_position(double cut_z_position)
{
    m_cut_position->set_default(cut_z_position);
}

void CutDialog::set_connector_type(Domain::CutConnectorType type)
{
    if (type == Domain::CutConnectorType::Undef) {
        m_dowel_btn->set_checked(false);
        m_plug_btn->set_checked(false);
        m_snap_btn->set_checked(false);
        return;
    }
    connector_type = type;
    switch (type) {
    case Domain::CutConnectorType::Dowel:
        m_dowel_btn->set_checked(true);
        break;
    case Domain::CutConnectorType::Plug:
        m_plug_btn->set_checked(true);
        break;
    case Domain::CutConnectorType::Snap:
        m_snap_btn->set_checked(true);
        break;
    default:
        break;
    }

    const bool is_snap = type == Domain::CutConnectorType::Snap;
    m_style_row->set_enabled(!is_snap);
    m_shape_row->set_enabled(!is_snap);
    m_snap_bulge_row->parent()->set_visible(is_snap);
    m_snap_space_row->parent()->set_visible(is_snap);

    if (type == Domain::CutConnectorType::Dowel) {
        m_prism_btn->set_checked(true);
        connector_style = Domain::CutConnectorStyle::Prism;
    }
    m_frustum_btn->set_enabled(type != Domain::CutConnectorType::Dowel);
}

void CutDialog::set_connector_style(Domain::CutConnectorStyle style)
{
    if (style == Domain::CutConnectorStyle::Undef) {
        m_frustum_btn->set_checked(false);
        m_prism_btn->set_checked(false);
        return;
    }
    connector_style = style;
    switch (style) {
    case Domain::CutConnectorStyle::Frustum:
        m_frustum_btn->set_checked(true);
        break;
    case Domain::CutConnectorStyle::Prism:
        m_prism_btn->set_checked(true);
        break;
    default:
        break;
    }
}

void CutDialog::set_connector_shape(Domain::CutConnectorShape shape)
{
    if (shape == Domain::CutConnectorShape::Undef) {
        m_triangle_btn->set_checked(false);
        m_square_btn->set_checked(false);
        m_hexagon_btn->set_checked(false);
        m_circle_btn->set_checked(false);
        return;
    }
    connector_shape = shape;
    switch (shape) {
    case Domain::CutConnectorShape::Triangle:
        m_triangle_btn->set_checked(true);
        break;
    case Domain::CutConnectorShape::Square:
        m_square_btn->set_checked(true);
        break;
    case Domain::CutConnectorShape::Hexagon:
        m_hexagon_btn->set_checked(true);
        break;
    case Domain::CutConnectorShape::Circle:
        m_circle_btn->set_checked(true);
        break;
    default:
        break;
    }
}

static void set_slider(
    SliderWithInput* slider,
    double min_val,
    double max_val,
    double step,
    double value,
    std::optional<double>(default_value) = std::nullopt
)
{
    // Round doubles for 2 digits to correct behavior of revert buttons
    auto round_2 = [](double value) -> double { return std::round(value * 100.0) / 100.0; };

    slider->set_begin_value(min_val);
    slider->set_end_value(max_val);
    slider->set_value(round_2(value));
    slider->set_step(step);
    if (default_value) {
        slider->set_default(round_2(default_value.value()));
    }
}

void CutDialog::set_groove_values(const Biz::Cut::Groove& m_groove, double max_elements_size)
{
    set_slider(
        m_groove_depth_value,
        1.,
        max_elements_size,
        0.01,
        m_groove.depth,
        m_groove.depth_init
    );
    double max_depth_tolerance = std::min(0.3 * m_groove.depth, 1.5);
    set_slider(m_groove_depth_tolerance, 0., max_depth_tolerance, 0.1, m_groove.depth_tolerance);

    set_slider(
        m_groove_width_value,
        1.,
        max_elements_size,
        0.01,
        m_groove.width,
        m_groove.width_init
    );
    double max_width_tolerance = std::min(0.3 * m_groove.width, 1.5);
    set_slider(m_groove_width_tolerance, 0., max_width_tolerance, 0.1, m_groove.width_tolerance);

    set_slider(
        m_flap_angle,
        30.,
        120.,
        1.,
        rad2deg(m_groove.flaps_angle),
        rad2deg(m_groove.flaps_angle_init)
    );
    set_slider(m_groove_angle, 0., 15., 1., rad2deg(m_groove.angle), rad2deg(m_groove.angle_init));
}

void CutDialog::set_connector_values(double max_size)
{
    const double depth_min_value =
        connector_type == Domain::CutConnectorType::Snap ? connector_size : 1.;
    const double max_tolerance = 0.5 * max_size;

    set_slider(m_connector_depth_value, depth_min_value, max_size, 0.1, connector_depth, 3.);
    set_slider(
        m_connector_depth_tolerance,
        0.,
        max_tolerance,
        0.01,
        connector_depth_tolerance,
        0.1
    );
    set_slider(m_connector_size_value, 1., max_size, 0.1, connector_size, 2.5);
    set_slider(m_connector_size_tolerance, 0., max_tolerance, 0.01, connector_size_tolerance, 0.);
    set_slider(m_connector_rotation, 0., 180., 1., connector_angle, 0.);

    set_slider(
        m_snap_bulge_proportion,
        5.,
        double(int(snap_space_proportion * 100)),
        1.,
        snap_bulge_proportion,
        15.
    );
    set_slider(m_snap_space_proportion, 10., 50., 1., snap_space_proportion, 30.);
}

void CutDialog::set_connector_values(
    std::optional<double> depth,
    std::optional<double> depth_tolerance,
    std::optional<double> half_size,
    std::optional<double> half_size_tolerance,
    std::optional<double> angle
)
{
    if (depth) {
        m_connector_depth_value->set_value(depth.value());
    }
    else {
        m_connector_depth_value->set_undef_value();
    }

    if (depth_tolerance) {
        m_connector_depth_tolerance->set_value(depth_tolerance.value());
    }
    else {
        m_connector_depth_tolerance->set_undef_value();
    }

    if (half_size) {
        m_connector_size_value->set_value(2. * half_size.value());
    }
    else {
        m_connector_size_value->set_undef_value();
    }

    if (half_size_tolerance) {
        m_connector_size_tolerance->set_value(2. * half_size_tolerance.value());
    }
    else {
        m_connector_size_tolerance->set_undef_value();
    }

    if (angle) {
        m_connector_rotation->set_value(rad2deg(angle.value()));
    }
    else {
        m_connector_rotation->set_undef_value();
    }

}

} // namespace Slic3r::App::Plater
