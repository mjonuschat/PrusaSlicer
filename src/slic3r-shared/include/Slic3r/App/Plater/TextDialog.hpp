///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoWindow.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/App/Yoga/AlignmentButtons.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Domain/ModelVolume.hpp" // ModelVolumeType
#include <vector>

namespace Slic3r {
// Limits for inputs
template<typename T> struct MinMax { T min; T max; };
template<typename T>
static bool apply(std::optional<T>& val, const MinMax<T>& limit) {
    if (!val.has_value()) return false;
    return apply<T>(*val, limit);
}
template<typename T>
static bool apply(T& val, const MinMax<T>& limit)
{
    if (val > limit.max) {
        val = limit.max;
        return true;
    }
    if (val < limit.min) {
        val = limit.min;
        return true;
    }
    return false;
}
}

namespace Slic3r::App::Yoga {
class LayoutButton;
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class TextDialog : public Yoga::GizmoWindow
{
public:
    TextDialog();

    struct Callbacks
    {
        std::function<void(const std::string&)> text_changed{ nullptr };
        std::function<void(const Domain::FontDescriptor& font_descriptor)> font_selection_changed{nullptr};

        std::function<void(double value)> height_changed{nullptr};
        std::function<void(double value)> depth_changed{nullptr};

        std::function<void(bool checked)> use_surface_checked{nullptr};
        std::function<void(bool checked)> per_glyph_checked{nullptr};
        std::function<void(const Domain::FontProp::Align& align)> align_changed{nullptr};
        std::function<void(double value)> char_gap_changed{nullptr};
        std::function<void(double value)> line_gap_changed{nullptr};
        std::function<void(double value)> boldness_changed{nullptr};
        std::function<void(double value)> skew_ratio_changed{nullptr};
        std::function<void(double value)> surface_distance_changed{nullptr};
        std::function<void(double value)> rotation_changed{nullptr};
        std::function<void(bool unlocked)> unlock_rotation{nullptr};
        std::function<void()> set_on_face_camera{nullptr};

        std::function<void(int index)> preset_selection_changed{nullptr};
        std::function<void()> save_preset_as{nullptr};
        std::function<void()> save_preset{nullptr};
        std::function<void()> rename_preset{nullptr};
        std::function<void()> delete_preset{nullptr};

        std::function<void(Domain::ModelVolumeType type)> operation_selection_changed{ nullptr };
    };

    Callbacks& callbacks();

    void update_units(bool use_inches);

    void set_editor(const std::string& text);
    void set_presets(const std::vector<std::string>& presets, int selected_preset_id);
    void set_fonts(const Domain::FontList& fonts);
    void set_font(const Domain::FontDescriptor& font, bool set_as_default);
    void set_height(double from, double to, double step, double step_fast, double height, double default_height);
    void set_depth(double from, double to, double step, double step_fast, double depth, double default_depth);

    void set_use_surface(bool checked, bool default_checked);
    void set_per_glyph(bool checked, bool default_checked);

    void set_align(const Domain::FontProp::Align& align, const Domain::FontProp::Align& align_default);

    void set_char_gap(double max_val, double step, double value, double default_value = 0.);
    void set_line_gap(double max_val, double step, double value, double default_value = 0.);
    void set_boldness(double max_val, double step, double value, double default_value = 0.);
    void set_skew_ratio(double max_val, double step, double value, double default_value = 0.);
    void set_surface_distance(double max_val, double step, double value, double default_value = 0.);
    void set_rotation(double max_val, double step, double value, double default_value = 0.);

    void set_enable_use_surface(bool enable);
    void set_enable_per_glyph(bool enable);
    void set_enable_line_gap(bool enable);
    void set_enable_surface_distance(bool enable);
    void set_warning(const std::string& warning);

    void set_operation(Domain::ModelVolumeType type);
    void show_part_specific_panel(bool show);
    void show_revert_buttons(bool show);

    void set_enable_all_except_font(bool enable);
private:
    void add_advanced_panel();
    void add_part_specific_panel();

    Yoga::Item* add_row(
        const std::string& title,
        Yoga::ItemPtr control,
        Yoga::Item* parent,
        const std::string& revert_button_tooltip = std::string(),
        const std::string& unit                  = std::string()
    );

private:
    Yoga::InputTextField* m_editor{nullptr};
    Yoga::LayoutButton* m_editor_warning{nullptr};

    Yoga::Passthrough<Yoga::ComboBox> m_font;
    // current OS enumerated fonts with styles(italic/bold) sorted alphanumericaly
    Domain::FontList m_fonts;
    Yoga::Passthrough<Yoga::ComboBox> m_style;

    Yoga::Passthrough<Yoga::InputTextWithSpin> m_height;
    Yoga::Passthrough<Yoga::InputTextWithSpin> m_depth;

    // vector of Text items used for mm/inch units
    // Will be updated on units sweetching
    std::vector<Yoga::Text*> m_units;

    Yoga::ToggleButton* m_advanced{nullptr};

    Yoga::Item* m_advanced_panel{nullptr};
    Yoga::Passthrough<Yoga::ToggleButton> m_use_surface;
    Yoga::Passthrough<Yoga::ToggleButton> m_per_glyph;
    Yoga::Passthrough<Yoga::AlignmentButtons> m_align;
    Yoga::Passthrough<Yoga::SliderWithInput> m_char_gap;
    Yoga::Passthrough<Yoga::SliderWithInput> m_line_gap;
    Yoga::Passthrough<Yoga::SliderWithInput> m_boldness;
    Yoga::Passthrough<Yoga::SliderWithInput> m_skew_ratio;
    Yoga::Passthrough<Yoga::SliderWithInput> m_surface_distance;
    Yoga::Passthrough<Yoga::SliderWithInput> m_rotation;
    Yoga::LayoutButton* m_lock_offset_btn{nullptr};

    Yoga::LayoutButton* m_set_on_face_camera_btn{nullptr};

    Yoga::Passthrough<Yoga::ComboBox> m_preset;
    Yoga::LayoutButton* m_save_as_new_btn{nullptr};
    Yoga::LayoutButton* m_save_btn{nullptr};
    Yoga::LayoutButton* m_rename_btn{nullptr};
    Yoga::LayoutButton* m_delete_btn{nullptr};

    Yoga::Item* m_part_specific_panel{nullptr};
    Yoga::Passthrough<Yoga::ComboBox> m_operation;

    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Plater
