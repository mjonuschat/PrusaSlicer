///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextDialog.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include <boost/assign.hpp>
#include <boost/bimap.hpp>

#include "Slic3r/Biz/I18N/I18N.hpp"
#include <fmt/format.h>

#include <imgui_internal.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace {
constexpr double MM_TO_INCH = 25.4;
constexpr double INCH_TO_MM = 1. / MM_TO_INCH;
constexpr double RAD_TO_DEG = 180.0 / M_PI;
constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double SURFACE_DISTANCE_STEP = 0.01;
} // namespace

namespace Slic3r::App::Plater {

bool set_debug = false;

TextDialog::Callbacks& TextDialog::callbacks()
{
    return m_callbacks;
}

TextDialog::TextDialog() : GizmoWindow(_u8L("Text"), Render::Icon::Text)
{
    content()->set_debug_border(set_debug);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size());

    m_editor = content()->emplace_back<InputTextField>();
    m_editor->set_height(100); // set multi-line mode
    m_editor->set_flags(m_editor->flags() | ImGuiInputTextFlags_Multiline);
    m_editor->callbacks().text_changed = [this]() {
        if (m_callbacks.text_changed)
            m_callbacks.text_changed(m_editor->text());
    };

    m_editor_warning = m_editor->emplace_back<LayoutButton>(
        "",
        Render::Icon::WarningMarker,
        "Some warning tooltips"
    );
    m_editor_warning->set_self_align(YGAlignFlexEnd);
    m_editor_warning->set_visible(false);

    m_font                                = Passthrough{std::make_unique<ComboBox>("Font name")};
    m_font->callbacks().selection_changed = [this](int index) {
        if (index >= m_fonts.size())
            return; // should not happend
        const Domain::FontDescriptor& font = m_fonts[index];
        set_font(font, false);
        if (m_callbacks.font_selection_changed)
            m_callbacks.font_selection_changed(font);
    };
    add_row(_u8L("Font"), m_font.release(), content(), _u8L("Revert font changes."));

    m_style                                = Passthrough{std::make_unique<ComboBox>("Font style")};
    m_style->callbacks().selection_changed = [this](int index) {
        size_t font_index = m_font->current_index() + index;
        if (m_callbacks.font_selection_changed && font_index < m_fonts.size())
            m_callbacks.font_selection_changed(m_fonts[font_index]);
    };
    add_row(_u8L("Style"), m_style.release(), content(), _u8L("Revert style changes."));

    m_height = Passthrough{
        std::make_unique<InputTextWithSpin>(std::make_unique<DoubleValidator>(0.1, 100.), 0.1, 1.)
    };
    m_height->callbacks().text_edited = [this]() {
        double height = std::stod(m_height->text());
        if (m_use_inches)
            height *= INCH_TO_MM;
        if (m_callbacks.height_changed)
            m_callbacks.height_changed(height);
    };
    add_row(_u8L("Height"), m_height.release(), content(), _u8L("Revert text size."), "mm");

    m_depth = Passthrough{
        std::make_unique<InputTextWithSpin>(std::make_unique<DoubleValidator>(0.1, 100.), 0.1, 1.)
    };
    m_depth->callbacks().text_edited = [this]() {
        double depth = std::stod(m_depth->text());
        if (m_use_inches)
            depth *= INCH_TO_MM;
        if (m_callbacks.depth_changed)
            m_callbacks.depth_changed(depth);
    };
    add_row(_u8L("Depth"), m_depth.release(), content(), _u8L("Revert embossed depth."), "mm");

    m_advanced = content()->emplace_back<ToggleButton>(_u8L("Advanced"));
    m_advanced->callbacks().checked_changed = [this](bool checked) {
        m_advanced_panel->set_visible(checked);
    };

    add_advanced_panel();

    add_separator(content());

    m_preset = Passthrough{std::make_unique<ComboBox>("Font preset")};
    m_preset->callbacks().selection_changed = [this](int index) {
        if (m_callbacks.preset_selection_changed)
            m_callbacks.preset_selection_changed(index);
    };
    add_row(_u8L("Preset"), m_preset.release(), content());

    std::unique_ptr<Item> buttons = std::make_unique<Item>();
    buttons->set_gap(gap_size());
    m_save_as_new_btn = buttons->emplace_back<LayoutButton>(
        "",
        Render::Icon::NewBtnIcon,
        _u8L("Save as new preset")
    );
    m_save_as_new_btn->callbacks().action = [this]() {
        if (m_callbacks.save_preset_as)
            m_callbacks.save_preset_as();
    };

    m_save_btn = buttons->emplace_back<LayoutButton>("", Render::Icon::TobBarSave, _u8L("Save preset"));
    m_save_btn->callbacks().action = [this]() {
        if (m_callbacks.save_preset)
            m_callbacks.save_preset();
    };

    m_rename_btn = buttons->emplace_back<LayoutButton>(
        std::string{},
        Render::Icon::SliderFloatEditBtnIcon,
        _u8L("Rename current preset.")
    );
    m_rename_btn->callbacks().action = [this]() {
        if (m_callbacks.rename_preset)
            m_callbacks.rename_preset();
    };

    m_delete_btn = buttons->emplace_back<LayoutButton>(
        std::string{},
        Render::Icon::DeleteBtnIcon,
        _u8L("Remove preset")
    );
    m_delete_btn->callbacks().action = [this]() {
        if (m_callbacks.delete_preset)
            m_callbacks.delete_preset();
    };

    // add_row("", std::move(buttons), nullptr, false);
    add_row({}, std::move(buttons), content());

    for (LayoutButton* btn :
         {m_save_as_new_btn, m_save_btn, m_rename_btn, m_delete_btn, m_lock_offset_btn})
    {
        btn->set_min_size(Vec2f(24.f, 24.f));
    }

    add_part_specific_panel();
}

void TextDialog::add_advanced_panel()
{
    m_advanced_panel = content()->emplace_back<Item>();
    m_advanced_panel->set_debug_border(set_debug);

    m_advanced_panel->set_visible(false);
    m_advanced_panel->set_orientation(Orientation::Vertical);
    m_advanced_panel->set_padding({content()->padding().left, 0});
    m_advanced_panel->set_gap(gap_size());

    m_use_surface = Passthrough{std::make_unique<ToggleButton>(_u8L("Use Surface"))};
    m_use_surface->set_tooltip(
        _u8L("If checked,\n\
model surface under the text's shape is shift in the embossing direction,\n\
otherwise text is flat and you have to deal with distance from surface.")
    );
    m_use_surface->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.use_surface_checked)
            m_callbacks.use_surface_checked(checked);
    };
    add_row("", m_use_surface.release(), m_advanced_panel, _u8L("Revert using of model surface."));

    m_per_glyph = Passthrough{std::make_unique<ToggleButton>(_u8L("Per Glyph"))};
    m_per_glyph->set_tooltip(
        _u8L("If checked,\n\
each letter(glyph) has an independent orthogonal projection,\n\
otherwise, the whole text has the same orthogonal projection.")
    );
    m_per_glyph->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.per_glyph_checked)
            m_callbacks.per_glyph_checked(checked);
    };
    add_row("", m_per_glyph.release(), m_advanced_panel, _u8L("Revert Transformation per glyph."));

    m_align                            = Passthrough{std::make_unique<AlignmentButtons>()};
    m_align->callbacks().align_changed = [this](const Domain::FontProp::Align& align) {
        if (m_callbacks.align_changed)
            m_callbacks.align_changed(align);
    };
    add_row(_u8L("Alignment"), m_align.release(), m_advanced_panel, _u8L("Revert alignment."));

    m_char_gap                            = Passthrough{std::make_unique<SliderWithInput>()};
    m_char_gap->callbacks().value_changed = [this](double value) {
        if (m_callbacks.char_gap_changed)
            m_callbacks.char_gap_changed(value);
    };
    // m_char_gap->set_tooltip(_u8L("Additional distance between characters"));
    add_row(
        _u8L("Char gap"),
        m_char_gap.release(),
        m_advanced_panel,
        _u8L("Revert gap between characters"),
        "mm"
    );

    m_line_gap                            = Passthrough{std::make_unique<SliderWithInput>()};
    m_line_gap->callbacks().value_changed = [this](double value) {
        if (m_callbacks.line_gap_changed)
            m_callbacks.line_gap_changed(value);
    };
    // m_line_gap->set_tooltip(_u8L("Additional distance between lines"));
    add_row(
        _u8L("Line gap"),
        m_line_gap.release(),
        m_advanced_panel,
        _u8L("Revert gap between lines"),
        "mm"
    );

    m_boldness                            = Passthrough{std::make_unique<SliderWithInput>()};
    m_boldness->callbacks().value_changed = [this](double value) {
        if (m_callbacks.boldness_changed)
            m_callbacks.boldness_changed(value);
    };
    // m_boldness->set_tooltip(_u8L("Tiny / Wide glyphs"));
    add_row(_u8L("Boldness"), m_boldness.release(), m_advanced_panel, _u8L("Undo boldness"), "mm");

    m_skew_ratio                            = Passthrough{std::make_unique<SliderWithInput>()};
    m_skew_ratio->callbacks().value_changed = [this](double value) {
        if (m_callbacks.skew_ratio_changed)
            m_callbacks.skew_ratio_changed(value);
    };
    m_skew_ratio->set_begin_value(-2);
    m_skew_ratio->set_end_value(2);
    m_skew_ratio->set_step(0.01);
    // m_skew_ratio->set_tooltip(_u8L("Italic strength ratio");
    add_row(_u8L("Skew ratio"), m_skew_ratio.release(), m_advanced_panel, _u8L("Undo letter's skew"));

    m_surface_distance = Passthrough{std::make_unique<SliderWithInput>()};
    m_surface_distance->set_begin_value(-2.);
    m_surface_distance->set_end_value(2.);
    m_surface_distance->set_step(SURFACE_DISTANCE_STEP);
    m_surface_distance->callbacks().value_changed = [this](double value) {
        if (m_use_inches)
            value *= INCH_TO_MM;
        if (m_callbacks.surface_distance_changed)
            m_callbacks.surface_distance_changed(value);
    };
    // m_surface_distance->set_tooltip( _u8L("Distance of the center of the text to the model surface.");
    add_row(
        _u8L("From surface"),
        m_surface_distance.release(),
        m_advanced_panel,
        _u8L("Undo translation"),
        "mm"
    );

    m_rotation                            = Passthrough{std::make_unique<SliderWithInput>()};
    m_rotation->callbacks().value_changed = [this](double value) {
        if (!m_use_radians)
            value *= DEG_TO_RAD;
        if (m_callbacks.rotation_changed)
            m_callbacks.rotation_changed(value);
    };
    // m_rotation->set_tooltip(_u8L("Rotate text Clock-wise.");
    Item* row = add_row(
        _u8L("Rotation"),
        m_rotation.release(),

        m_advanced_panel,
        _u8L("Undo rotation"),
        std::string("°")
    );
    m_lock_offset_btn = row->emplace_back<LayoutButton>("", Render::Icon::Lock); // Note: for tooltip need externaly set value
    m_lock_offset_btn->set_self_align(YGAlignCenter);
    m_lock_offset_btn->set_checkable(true);
    m_lock_offset_btn->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.unlock_rotation)
            m_callbacks.unlock_rotation(checked);
        m_lock_offset_btn->set_icon(checked ? Render::Icon::Unlock : Render::Icon::Lock);
        m_lock_offset_btn->set_tooltip(
            checked ? _u8L("Lock the text's rotation when moving text along the object's surface.") :
                      _u8L("Unlock the text's rotation when moving text along the object's surface.")
        );
    };

    for (SliderWithInput* input :
         {m_char_gap.get(),
          m_line_gap.get(),
          m_boldness.get(),
          m_skew_ratio.get(),
          m_surface_distance.get(),
          m_rotation.get()})
    {
        input->set_input_width(40);
    }

    m_set_on_face_camera_btn = m_advanced_panel->emplace_back<LayoutButton>(
        _u8L("Set text to face camera")
    );
    m_set_on_face_camera_btn->set_background_color({43, 43, 43});
    m_set_on_face_camera_btn->callbacks().action = [this]() {
        if (m_callbacks.set_on_face_camera)
            m_callbacks.set_on_face_camera();
    };

    // update limits
    update_units(true); update_units(false);
    update_angle(true); update_angle(false);
}

namespace {
std::vector<std::string> get_operation_names() {
    // NOTE: odred must match to function to_type
    return {
        _u8L("Join with object"), // index 0
        _u8L("Cut from object"), // 1
        _u8L("Modify object") // 2
    };
}

Domain::ModelVolumeType to_type(size_t operation_index) {
    switch (operation_index) {
    case 0: return Domain::ModelVolumeType::MODEL_PART;
    case 1: return Domain::ModelVolumeType::NEGATIVE_VOLUME;
    case 2: return Domain::ModelVolumeType::PARAMETER_MODIFIER;
    }
    // should not appear
    return Domain::ModelVolumeType::MODEL_PART;
}
size_t to_operation_index(Domain::ModelVolumeType type) {
    switch (type) {
    case Domain::ModelVolumeType::MODEL_PART: return 0;
    case Domain::ModelVolumeType::NEGATIVE_VOLUME: return 1;
    case Domain::ModelVolumeType::PARAMETER_MODIFIER: return 2;
    default: return 0; // should not appear
    }
}
}

void TextDialog::add_part_specific_panel()
{
    m_part_specific_panel = content()->emplace_back<Item>();
    m_part_specific_panel->set_debug_border(set_debug);
    m_part_specific_panel->set_orientation(Orientation::Vertical);
    m_part_specific_panel->set_gap(gap_size());
    add_separator(m_part_specific_panel);
    m_operation = Passthrough(std::make_unique<ComboBox>(get_operation_names()));
    m_operation->callbacks().selection_changed = [this](int index) {
        if (m_callbacks.operation_selection_changed)
            m_callbacks.operation_selection_changed(to_type(index));
    };
    add_row(_u8L("Operation"), m_operation.release(), m_part_specific_panel);

    m_part_specific_panel->set_visible(false);
}

void TextDialog::set_operation(Domain::ModelVolumeType type){
    m_operation->set_current_index(to_operation_index(type));
}

Item* TextDialog::add_row(
    const std::string& title,
    Yoga::ItemPtr control,
    Yoga::Item* parent,
    const std::string& revert_tooltip,
    const std::string& unit
)
{
    Item* row = parent->emplace_back<Item>();
    row->set_debug_border(set_debug);
    row->set_gap(3);

    Text* text = row->emplace_back<Text>(title);
    text->set_width_percent(25);
    text->set_self_align(YGAlignCenter);
    text->set_debug_border(set_debug);

    Item* revert_space = row->emplace_back<Item>();
    revert_space->set_justify_content(YGJustifyFlexEnd);
    if (!revert_tooltip.empty()) {
        LayoutButton* revert = revert_space->emplace_back<LayoutButton>(
            "",
            Render::Icon::DSRevert,
            revert_tooltip
        );
        revert->set_self_align(YGAlignCenter);
        revert->set_min_size(Vec2f{ 20.f,20.f });
        revert->set_visible(false);
        // set revert button fot control
        if (RevertableControl* revertable_control = dynamic_cast<RevertableControl*>(control.get()))
            revertable_control->set_revert_button(revert);
    }

    Item* box = row->emplace_back<Item>();
    box->set_debug_border(set_debug);
    box->set_width_percent(65);
    box->set_gap(gap_size());
    control->set_flex_grow(1);
    control->set_debug_border(set_debug);
    box->append(std::move(control));

    if (!unit.empty()) {
        Text* unit_text{nullptr};
        unit_text = box->emplace_back<Text>(unit);
        if (unit == "mm") {
            // Means that unit will be changed in respect to the "use_inches" app_config option
            // so, add it to the m_units vector
            m_units.emplace_back(unit_text);
        } else if(unit == "°") {
            m_angle_unit = unit_text;
        }
        unit_text->set_self_align(YGAlignCenter);
        return box;
    }

    return row;
}

void TextDialog::show_part_specific_panel(bool show)
{
    m_part_specific_panel->set_visible(show);
}

namespace {
static void set_spin_limits(
    InputTextWithSpin* spin,
    double from,
    double to,
    double step,
    double step_fast)
{
    DoubleValidator* validator = dynamic_cast<DoubleValidator*>(spin->validator());
    validator->set_from(from);
    validator->set_to(to);
    spin->set_step(step);
    spin->set_step_fast(step_fast);
}
void set_limist(SliderWithInput* slider, double max_val, double step)
{
    slider->set_begin_value(-max_val);
    slider->set_end_value(max_val);
    slider->set_step(step);
}
} // namespace

void TextDialog::update_units(bool use_inches)
{
    if (m_use_inches == use_inches)
        return; // already setted
    m_use_inches = use_inches;

    for (Text* unit : m_units) {
        unit->set_text(use_inches ? _u8L("in") : _u8L("mm"));
    }
    // update limits
    if (use_inches) {
        set_spin_limits(m_height.get(), .005, 4., .005, .05);
        set_spin_limits(m_depth.get(), .005, 4., .005, .05);
        set_limist(m_char_gap.get(), .2, .005);
        set_limist(m_line_gap.get(), .2, .005);
        set_limist(m_boldness.get(), .2, .005);
    } else {
        set_spin_limits(m_height.get(), .1, 100., .1, 1.);
        set_spin_limits(m_depth.get(), .1, 100., .1, 1.);
        set_limist(m_char_gap.get(), 5., .1);
        set_limist(m_line_gap.get(), 5., .1);
        set_limist(m_boldness.get(), 5., .1);
    }
}


void TextDialog::update_angle(bool use_radians)
{
    if (m_use_radians == use_radians)
        return; // already setted
    m_use_radians = use_radians;

    m_angle_unit->set_text(use_radians ? _u8L("rad") : _u8L("°"));
    if (use_radians) {
        m_rotation->set_begin_value(-M_PI);
        m_rotation->set_end_value(M_PI);
        m_rotation->set_step(.02);
    } else {
        m_rotation->set_begin_value(-180.);
        m_rotation->set_end_value(180.);
        m_rotation->set_step(1.);
    }
}

void TextDialog::set_editor(const std::string& text)
{
    m_editor->set_text(text);
}

void TextDialog::set_presets(const std::vector<std::string>& presets, int selected_preset_id)
{
    m_preset->set_items(presets);
    m_preset->set_current_index(selected_preset_id);
}

void TextDialog::set_fonts(const Domain::FontList& fonts)
{
    m_fonts = fonts;
    std::vector<std::string> names;
    names.reserve(fonts.size());
    for (const Domain::FontDescriptor& font : fonts) {
        names.push_back(font.name);
    }
    m_font->set_items(names);
}

namespace {
std::string get_first_word(const std::string& sentence) {
    // Find the position of the first space character.
    // std::string::npos is returned if no space is found.
    size_t spacePos = sentence.find(' ');

    if (spacePos == std::string::npos) {
        // If no space is found, the entire string is the first word.
        return sentence;
    }
    // Space is found, return the substring from the beginning
    // up to the position of the space.
    return sentence.substr(0, spacePos);
}

bool starts_with(const std::string& sentence, const std::string& word){
    return sentence.rfind(word, 0) == 0; // pos=0 limits the search to the prefix
}
} // namespace

void TextDialog::set_font(const Domain::FontDescriptor& font, bool set_as_default)
{
    if (m_fonts.empty())
        return; // First call function TextDialog::set_fonts()

    auto unknown_font = [&]() {
        m_font->set_current_index(0);
        set_enable_all_except_font(false);
        m_style->set_items({_u8L("Not available")});
        m_style->set_current_index(0);
    };

    if (font.type != m_fonts.front().type)
        // not current type
        return unknown_font();

    auto font_it = std::find_if(m_fonts.begin(), m_fonts.end(), [&font](const Domain::FontDescriptor& f) {
        return f.path == font.path; });
    if (font_it == m_fonts.end())
        // not in known font but from same Operating system
        return unknown_font();
    
    std::string name = get_first_word(font_it->name);
    auto start_style_it = font_it;
    while (start_style_it != m_fonts.begin() && starts_with((--start_style_it)->name, name));
    if (start_style_it != m_fonts.begin() || !starts_with(start_style_it->name, name))
        ++start_style_it;

    auto end_style_it = font_it;
    while (end_style_it != m_fonts.end() && starts_with((++end_style_it)->name, name));

    m_font->set_current_index(start_style_it - m_fonts.begin());
    if (set_as_default)
        m_font->set_default(start_style_it - m_fonts.begin());

    std::vector<std::string> style_names;
    style_names.reserve(end_style_it - start_style_it);
    for (auto style_it = start_style_it; style_it != end_style_it; ++style_it) {
        if (style_it->name.size() <= name.size()){
            style_names.push_back(_u8L("Regular"));
        } else {
            style_names.push_back(style_it->name.substr(name.size() + 1));
        }
    }

    m_style->set_items(style_names);
    m_style->set_current_index(font_it - start_style_it);
    if (set_as_default)
        m_style->set_default(font_it - start_style_it);
    // known font
    set_enable_all_except_font(true);
}

namespace {
void set_value(InputTextWithSpin* spin, double value_in_mm, double default_value_in_mm, bool use_inches) {
    if (use_inches) { // convert values to inches
        value_in_mm *= MM_TO_INCH;
        default_value_in_mm *= MM_TO_INCH;
    }
    spin->set_text(fmt::format("{:.10g}", value_in_mm));
    spin->set_default(default_value_in_mm);
}
} // namespace

void TextDialog::set_text_height(double height_in_mm, double default_height) {
    set_value(m_height.get(), height_in_mm, default_height, m_use_inches);
}

void TextDialog::set_depth(double depth_in_mm, double default_depth) {
    set_value(m_depth.get(), depth_in_mm, default_depth, m_use_inches);
}

void TextDialog::set_use_surface(bool checked, bool default_checked)
{
    m_use_surface->set_checked(checked);
    m_use_surface->set_default(default_checked);
}

void TextDialog::set_per_glyph(bool checked, bool default_checked)
{
    m_per_glyph->set_checked(checked);
    m_per_glyph->set_default(default_checked);
}

void TextDialog::set_align(const Domain::FontProp::Align& align, const Domain::FontProp::Align& align_default)
{
    m_align->set_align(align);
    m_align->set_default(align_default);
}
namespace {
void set_value(SliderWithInput* slider, double value, double default_value, bool use_inches)
{
    if (use_inches) {
        slider->set_value(value * MM_TO_INCH);
        slider->set_default(default_value * MM_TO_INCH);
    } else {
        slider->set_value(value);
        slider->set_default(default_value);
    }
}
} // namespace

void TextDialog::set_char_gap(double char_gap_in_mm, double default_char_gap_in_mm) {
    set_value(m_char_gap.get(), char_gap_in_mm, default_char_gap_in_mm, m_use_inches);
}
void TextDialog::set_line_gap(double line_gap_in_mm, double default_line_gap_in_mm) {
    set_value(m_line_gap.get(), line_gap_in_mm, default_line_gap_in_mm, m_use_inches);
}
void TextDialog::set_boldness(double boldness_in_mm, double default_boldness_in_mm) {
    set_value(m_boldness.get(), boldness_in_mm, default_boldness_in_mm, m_use_inches);
}
void TextDialog::set_skew_ratio(double value, double default_value) {
    set_value(m_skew_ratio.get(), value, default_value, false); // unit less value
}
void TextDialog::set_surface_distance(double maximal_value_in_mm, double surface_distance_in_mm, double default_surface_distance_in_mm) {
    if (m_use_inches) {
        maximal_value_in_mm *= MM_TO_INCH;
    }

    // use N steps from zero to be able set to zero
    maximal_value_in_mm = std::ceil(maximal_value_in_mm / SURFACE_DISTANCE_STEP) * SURFACE_DISTANCE_STEP;

    m_surface_distance->set_begin_value(-maximal_value_in_mm);
    m_surface_distance->set_end_value(maximal_value_in_mm);
    set_value(m_surface_distance.get(), surface_distance_in_mm, default_surface_distance_in_mm, m_use_inches);    
}

void TextDialog::set_rotation(const std::optional<float>& angle_in_rad, const std::optional<float>& default_angle_in_rad){
    double v = angle_in_rad.value_or(0.);
    double d = default_angle_in_rad.value_or(0.);
    if (!m_use_radians) {
        v *= RAD_TO_DEG;
        d *= RAD_TO_DEG;
    }
    set_value(m_rotation.get(), v, d, false); // unit already converted
}

void TextDialog::set_rotation_lock(bool lock)
{
    m_lock_offset_btn->set_checked(lock);
}

void TextDialog::set_enable_all_except_font(bool enable)
{
    Item* font_row = m_font->parent_item()->parent_item();
    for (auto row : content()->items()) {
        if (row != font_row)
            row->set_enabled(enable);
    }
}

static void enable_row_with_control(Item* control, bool enable)
{
    control->parent_item()->parent_item()->set_enabled(enable);
}

void TextDialog::set_enable_use_surface(bool enable)
{
    enable_row_with_control(m_use_surface.get(), enable);
}

void TextDialog::set_enable_per_glyph(bool enable)
{
    enable_row_with_control(m_per_glyph.get(), enable);
}

void TextDialog::set_enable_line_gap(bool enable)
{
    enable_row_with_control(m_line_gap.get(), enable);
}

void TextDialog::set_enable_surface_distance(bool enable)
{
    enable_row_with_control(m_surface_distance.get(), enable);
}

void TextDialog::set_warning(const std::string& warning)
{
    m_editor_warning->set_visible(!warning.empty());
    m_editor_warning->set_tooltip(warning);
}

void TextDialog::show_revert_buttons(bool show)
{
    if (m_font->has_valid_default() != show) {
        m_font->validate_default(show);
        m_style->validate_default(show);

        m_height->validate_default(show);
        m_depth->validate_default(show);

        m_use_surface->validate_default(show);
        m_per_glyph->validate_default(show);
        m_align->validate_default(show);
        m_char_gap->validate_default(show);
        m_line_gap->validate_default(show);
        m_boldness->validate_default(show);
        m_skew_ratio->validate_default(show);
        m_surface_distance->validate_default(show);
        m_rotation->validate_default(show);
    }
}

} // namespace Slic3r::App::Plater
