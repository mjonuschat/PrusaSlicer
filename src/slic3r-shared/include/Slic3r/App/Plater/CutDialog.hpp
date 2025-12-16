///|/ Copyright (c) Prusa Research 2025  Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/Domain/CutConnector.hpp"
#include "Slic3r/Biz/Utils/CutUtils.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
class ToggleButton;
class InputTextField;
class Text;
class SliderWithInput;

class PartProcessingRow : public Item
{
public:
    explicit PartProcessingRow(const std::string& part_name);
    virtual ~PartProcessingRow();

    enum class Type
    {
        Object,
        Part
    };

    enum class Action
    {
        Keep,
        PlaceOnCut,
        Flip
    };

    struct Callbacks
    {
        std::function<void(bool checked)> checked_changed{nullptr};
        std::function<void(Action action)> part_action_changed{nullptr};
    };

    Callbacks& callbacks();
    void set_as_part(bool is_part);
    void set_enabled_buttons(bool enabled);
    bool is_checked() const;

private:
    Yoga::ToggleButton* m_part_checker{nullptr};
    Yoga::ButtonGroup m_group;
    Yoga::LayoutButton* m_keep_btn{nullptr};
    Yoga::LayoutButton* m_place_on_cut_btn{nullptr};
    Yoga::LayoutButton* m_flip_btn{nullptr};

    Callbacks m_callbacks;

    std::string m_name;
    Type m_type{Type::Object};
    Action m_act{Action::Keep}; //?
};

} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class CutDialog : public Yoga::GizmoDialog
{
public:
    CutDialog();

    struct Callbacks
    {
        std::function<void(double value)> z_changed{nullptr};
        std::function<void()> mode_changed{nullptr};
        std::function<void()> reset_connectors{nullptr};
        std::function<void()> flip_cut_plane{nullptr};
        std::function<void()> perform{nullptr};

        std::function<void(double value)> groove_depth_value_changed{nullptr};
        std::function<void(double value)> groove_depth_tolerance_changed{nullptr};
        std::function<void(double value)> groove_width_value_changed{nullptr};
        std::function<void(double value)> groove_width_tolerance_changed{nullptr};
        std::function<void(double value)> flap_angle_changed{nullptr};
        std::function<void(double value)> groove_angle_changed{nullptr};
    };

    Callbacks& callbacks();

    void set_build_size(Domain::Vec3d tbb_size);
    void set_current_connetor_type(Domain::CutConnectorType type);
    void set_current_connetor_style(Domain::CutConnectorStyle style);
    void set_current_connetor_shape(Domain::CutConnectorShape shape);
    void set_groove_values(const Biz::Cut::Groove& m_groove, double m_max_elements_size);

public:

    bool is_planar_cut_mode{ true };
    bool keep_as_parts{ false };
    bool keep_upper{ true };
    bool keep_lower{ true };
    bool place_on_cut_upper{ true };
    bool place_on_cut_lower{ false };
    bool flip_upper{ false };
    bool flip_lower{ false };

private:
    void init_connectors_input_panel();
    void init_cut_plane_input_panel();
    void add_connectors_help_panel();
    void add_cut_plane_help_panel();
    void add_groove_input_panel();
    void add_cut_settings();

    // check state of teh cut settings and show warning line or "Perform" button
    void update_state();
    void update_panels_visibility();

    // return an Item(box) where some control can be placed
    Yoga::Item*
    add_row(const std::string& title, Yoga::Item* parent, const std::string& unit = std::string());

private:
    Yoga::Item* m_connectors_input_panel{nullptr};
    Yoga::ButtonGroup m_connector_type_group;
    Yoga::LayoutButton* m_plug_btn{nullptr};
    Yoga::LayoutButton* m_dowel_btn{nullptr};
    Yoga::LayoutButton* m_snap_btn{nullptr};
    Yoga::ButtonGroup m_connector_style_group;
    Yoga::LayoutButton* m_prism_btn{nullptr};
    Yoga::LayoutButton* m_frustum_btn{nullptr};
    Yoga::ButtonGroup m_connector_shape_group;
    Yoga::LayoutButton* m_triangle_btn{nullptr};
    Yoga::LayoutButton* m_square_btn{nullptr};
    Yoga::LayoutButton* m_hexagon_btn{nullptr};
    Yoga::LayoutButton* m_circle_btn{nullptr};
    Yoga::SliderWithInput* m_connector_depth_value{nullptr};
    Yoga::SliderWithInput* m_connector_depth_tolerance{nullptr};
    Yoga::SliderWithInput* m_connector_size_value{nullptr};
    Yoga::SliderWithInput* m_connector_size_tolerance{nullptr};
    Yoga::SliderWithInput* m_connector_rotation{nullptr};

    Yoga::Item* m_cut_plane_input_panel{nullptr};
    Yoga::ButtonGroup m_mode_group;
    Yoga::LayoutButton* m_planar_mode_btn{nullptr};
    Yoga::Text* m_build_volume{nullptr};
    Yoga::InputTextField* m_cut_position{nullptr};

    Yoga::Item* m_groove_input_panel{nullptr};
    Yoga::SliderWithInput* m_groove_depth_value{nullptr};
    Yoga::SliderWithInput* m_groove_depth_tolerance{nullptr};
    Yoga::SliderWithInput* m_groove_width_value{nullptr};
    Yoga::SliderWithInput* m_groove_width_tolerance{nullptr};
    Yoga::SliderWithInput* m_flap_angle{nullptr};
    Yoga::SliderWithInput* m_groove_angle{nullptr};

    Yoga::ButtonGroup m_cut_into_group;
    Yoga::LayoutButton* m_cut_into_objects_btn{nullptr};
    Yoga::LayoutButton* m_cut_into_parts_btn{nullptr};
    Yoga::PartProcessingRow* m_part_A{nullptr};
    Yoga::PartProcessingRow* m_part_B{nullptr};

    Yoga::Text* m_warnings_line{nullptr};
    Yoga::LayoutButton* m_perform_btn{nullptr};

    Callbacks m_callbacks;

    // vector of Text items used for mm/inch units
    // Will be updated on units sweetching
    std::vector<Yoga::Text*> m_units;

    bool m_imperial_units{false};
    bool m_connectors_editing{false};

    Domain::CutConnectorType m_current_connetor_type{Domain::CutConnectorType::Plug};
    Domain::CutConnectorStyle m_current_connetor_style{Domain::CutConnectorStyle::Prism};
    Domain::CutConnectorShape m_current_connetor_shape{Domain::CutConnectorShape::Circle};
};
} // namespace Slic3r::App::Plater
