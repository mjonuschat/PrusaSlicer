///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/SvgDialog.hpp"
#include "Slic3r/App/Plater/DialogUtils.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace {
// Constants 
constexpr double SURFACE_DISTANCE_STEP = 0.01;
constexpr double ANGLE_DEG_STEP = .1;
constexpr double ANGLE_RAD_STEP = .01;
}

namespace Slic3r::App::Plater {
const double SvgDialog::MIN_DEPTH{1e-3};
const double SvgDialog::MAX_DEPTH{1e2}; 
const double SvgDialog::MIN_HEIGHT{1e-3};
const double SvgDialog::MAX_HEIGHT{1e3};
const double SvgDialog::MIN_WIDTH{1e-3};
const double SvgDialog::MAX_WIDTH{1e3};

SvgDialog::SvgDialog() : GizmoWindow(_u8L("SVG emboss"), Render::Icon::Svg)
{
    content()->set_debug_border(m_set_debug);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size());

    m_filename = content()->emplace_back<Text>("File");
    auto suffix = m_filename->emplace_back<Text>(".svg");
    suffix->set_text_color(ImColor(0.4f, 0.4f, 0.4f, 1.0f )); // COL_GREY_LIGHT
    m_reload = m_filename->emplace_back<LayoutButton>("", Render::Icon::Reload, "Reload");
    m_reload->set_min_size(Vec2f{ 15.f,15.f });
    m_reload->callbacks().action = [this]() {
        if (m_callbacks.reload_file) m_callbacks.reload_file(); };
    
    auto options = m_filename->emplace_back<ComboBox>(std::initializer_list<std::string>{
            _u8L("Change file") + " ..",
            _u8L("Forget the filepath"),
            _u8L("Bake"),
            _u8L("Save as") + " .." });
    options->callbacks().selection_changed = [this, options](int index){
            switch (index) { // call action
            case 0: if (m_callbacks.change_file)     m_callbacks.change_file();     break;
            case 1: if (m_callbacks.forgot_filepath) m_callbacks.forgot_filepath(); break;
            case 2: if (m_callbacks.bake)            m_callbacks.bake();            break;
            case 3: if (m_callbacks.save_as)         m_callbacks.save_as();         break;
            }
            options->set_editable(true);
            options->set_current_label("");
            options->set_editable(false);
        };
    options->set_editable(true);
    options->set_current_label("");
    options->set_editable(false);
    options->set_min_size(Vec2f{ 20.f, 20.f });
    options->set_flags(ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview);

    m_warning = m_filename->emplace_back<LayoutButton>("", Render::Icon::WarningMarker, "Some warning");
    m_warning->set_self_align(YGAlignFlexStart);
    m_warning->set_min_size(Vec2f{ 20.f,20.f });
    m_warning->set_visible(false);

    add_separator(content());

    add_row_with_spin_double(
        _u8L("Depth"),
        content(),
        &m_depth,
        _u8L("mm"),
        "", // no revert button
        MIN_DEPTH,
        MAX_DEPTH,
        0.1,
        1.
    );
    m_depth->callbacks().text_edited = [this]()
    {
        if (m_callbacks.depth_changed)
            m_callbacks.depth_changed(std::stod(m_depth->text()));
    };
    std::string no_revert;

    add_row_with_spin_double(
        _u8L("Width"),
        content(),
        &m_width,
        _u8L("mm"),
        _u8L("Revert width"),
        MIN_DEPTH,
        MAX_DEPTH,
        1.,
        5.
    );
    m_width->callbacks().text_edited = [this]()
    {
        if (m_callbacks.size_changed)
            m_callbacks.size_changed(Domain::Vec2d{std::stod(m_width->text()), 0.});
    };

    auto height = add_row_with_spin_double(
        _u8L("Height"),
        content(),
        &m_height,
        _u8L("mm"),
        _u8L("Revert height"),
        MIN_DEPTH,
        MAX_DEPTH,
        1.,
        5.
    );
    m_height->callbacks().text_edited = [this]()
    {
        if (m_callbacks.size_changed)
            m_callbacks.size_changed(Domain::Vec2d{0., std::stod(m_height->text())});
    };

    m_lock_size_btn = height->emplace_back<LayoutButton>("", Render::Icon::Lock); // Note: for tooltip need externaly set value
    m_lock_size_btn->set_min_size(Vec2f{ 20.f,20.f });
    m_lock_size_btn->set_self_align(YGAlignCenter);
    m_lock_size_btn->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.unlock_size)
            m_callbacks.unlock_size(checked);
        m_lock_size_btn->set_icon(checked ? Render::Icon::Unlock : Render::Icon::Lock);
        m_lock_size_btn->set_tooltip(checked ? 
            _u8L("Keep current aspect ration.") : 
            _u8L("Free size changing."));
    };
    m_lock_size_btn->set_checkable(false); m_lock_size_btn->set_checkable(true); // set valid tooltip    

    m_use_surface = Passthrough{ std::make_unique<ToggleButton>() };
    m_use_surface->set_tooltip(
        _u8L("If checked,\n\
model surface under the text's shape is shift in the embossing direction,\n\
otherwise text is flat and you have to deal with distance from surface.")
);
    m_use_surface->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.use_surface_checked)
            m_callbacks.use_surface_checked(checked);
        };
    add_row(_u8L("Use Surface"), m_use_surface.release(), content(), no_revert);

    add_row_with_slider(
        content(),
        &m_surface_distance,
        _u8L("From surface"),
        _u8L("mm"),
        _u8L("Revert")
    );
    m_surface_distance->set_begin_value(-2.);
    m_surface_distance->set_end_value(2.);
    m_surface_distance->set_step(SURFACE_DISTANCE_STEP);
    m_surface_distance->set_default(0.);
    m_surface_distance->callbacks().value_changed = [this](double value)
    {
        if (m_use_inch)
            value *= INCH_TO_MM;
        if (m_callbacks.surface_distance_changed)
            m_callbacks.surface_distance_changed(value);
    };

    Item* rotation_row =
        add_row_with_slider(content(), &m_rotation, _u8L("Rotation"), _u8L("°"), _u8L("Revert"));
    m_rotation->set_begin_value(-180.);
    m_rotation->set_end_value(180.);
    m_rotation->set_step(ANGLE_DEG_STEP);
    m_rotation->set_default(0.);
    m_rotation->callbacks().value_changed = [this](double value)
    {
        if (m_use_deg)
            value *= DEG_TO_RAD;
        if (m_callbacks.rotation_changed)
            m_callbacks.rotation_changed(value);
    };

    m_lock_rotation_btn = rotation_row->emplace_back<LayoutButton>("", Render::Icon::Lock); // Note: for tooltip need externaly set value
    m_lock_rotation_btn->set_min_size(Vec2f{ 20.f,20.f });
    m_lock_rotation_btn->set_self_align(YGAlignCenter);
    m_lock_rotation_btn->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.unlock_rotation)
            m_callbacks.unlock_rotation(checked);
        m_lock_rotation_btn->set_icon(checked ? Render::Icon::Unlock : Render::Icon::Lock);
        m_lock_rotation_btn->set_tooltip(
            checked ? _u8L("Lock for the 'up vector' during move above surface.") :
                      _u8L("Free move above surface (can rotate around emboss axe).")
        );
    };
    m_lock_rotation_btn->set_checkable(false); m_lock_rotation_btn->set_checkable(true); // set valid tooltip

    m_mirror_x = Passthrough{ std::make_unique<LayoutButton>("", Render::Icon::ReflectionX) };
    m_mirror_x->set_min_size(Vec2f{ 30.f, 20.f });
    m_mirror_x->callbacks().action = [this]() {
        if (m_callbacks.mirror_x)
            m_callbacks.mirror_x();
    };
    Item* mirror_row = add_row(_u8L("Mirror"), m_mirror_x.release(), content(), no_revert);
    m_mirror_y = mirror_row->emplace_back<LayoutButton>("", Render::Icon::ReflectionY);
    m_mirror_y->set_min_size(Vec2f{ 30.f, 20.f });
    m_mirror_y->callbacks().action = [this]() {
        if (m_callbacks.mirror_y)
            m_callbacks.mirror_y();
    };
    mirror_row->set_visible(false); // Temporary hide mirror until mirror in PS will be ready
    
    m_face_the_camera_btn = content()->emplace_back<LayoutButton>(
        _u8L("Face the camera")
    );
    m_face_the_camera_btn->set_background_color({ 43, 43, 43 });
    m_face_the_camera_btn->callbacks().action = [this]() {
        if (m_callbacks.face_the_camera)
            m_callbacks.face_the_camera();
    };

    add_separator(content());

    // operation
    add_part_specific_panel();

    // update limits
    update_units(true); update_units(false);
    update_angle(false); update_angle(true);
}

SvgDialog::Callbacks& SvgDialog::callbacks() { return m_callbacks; }

void SvgDialog::set_warning(const std::string& warning)
{
    m_warning->set_visible(!warning.empty());
    m_warning->set_tooltip(warning);
}

// Round doubles for 1 digits to correct behavior of revert buttons
static double round_1(double value)
{
    return std::round(value * 10.0) / 10.0;
};

void SvgDialog::set_filename(const std::string& filename) { m_filename->set_text(filename); }
void SvgDialog::set_enable_reload_from_disk(bool enable) { m_reload->set_visible(enable); }
void SvgDialog::set_size(const Domain::Vec2d& size, const Domain::Vec2d& size_original) {
    m_width->set_text(fmt::format("{:.1f}", size.x()));
    m_width->set_default(round_1(size_original.x()));
    m_height->set_text(fmt::format("{:.1f}", size.y()));
    m_height->set_default(round_1(size_original.y()));
}
void SvgDialog::set_size_lock(bool lock) { m_lock_size_btn->set_checked(lock); }

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
    // should not appear
    default:return Domain::ModelVolumeType::MODEL_PART;
    }
}
int to_operation_index(Domain::ModelVolumeType type) {
    switch (type) {
    case Domain::ModelVolumeType::MODEL_PART: return 0;
    case Domain::ModelVolumeType::NEGATIVE_VOLUME: return 1;
    case Domain::ModelVolumeType::PARAMETER_MODIFIER: return 2;
    // should not appear
    default: return 0;
    }
}
}

void SvgDialog::add_part_specific_panel()
{
    m_part_specific_panel = content()->emplace_back<Item>();
    m_part_specific_panel->set_debug_border(m_set_debug);
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

void SvgDialog::set_operation(Domain::ModelVolumeType type){
    m_operation->set_current_index(to_operation_index(type));
}

Item* SvgDialog::add_row(
    const std::string& title,
    Yoga::ItemPtr control,
    Yoga::Item* parent,
    const std::string& revert_tooltip,
    const std::string& unit
)
{
    Item* row = parent->emplace_back<Item>();
    row->set_debug_border(m_set_debug);
    row->set_gap(3);

    Text* text = row->emplace_back<Text>(title);
    text->set_width_percent(25);
    text->set_self_align(YGAlignCenter);
    text->set_debug_border(m_set_debug);

    if (!revert_tooltip.empty()) {
        Item* revert_space = row->emplace_back<Item>();
        revert_space->set_justify_content(YGJustifyFlexEnd);
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
    box->set_debug_border(m_set_debug);
    box->set_width_percent(65);
    box->set_gap(gap_size());
    control->set_flex_grow(1);
    control->set_debug_border(m_set_debug);
    box->append(std::move(control));

    if (!unit.empty()) {
        Text* unit_text{nullptr};
        unit_text = box->emplace_back<Text>(unit);
        if (unit == "mm") {
            // Means that unit will be changed in respect to the "use_inches" app_config option
            // so, add it to the m_units vector
            m_units.emplace_back(unit_text);
        }
        unit_text->set_self_align(YGAlignCenter);
        return box;
    }

    return row;
}

void SvgDialog::show_part_specific_panel(bool show)
{
    m_part_specific_panel->set_visible(show);
}

void SvgDialog::update_units(bool use_inch)
{
    if (use_inch == m_use_inch)
        return;

    m_use_inch = use_inch;
    std::string unit_text = use_inch ? _u8L("in") : _u8L("mm");
    for (Text* unit : m_units) {
        unit->set_text(unit_text);
    }
    // update limits
    if (use_inch) {
        set_spin_limits(m_depth, .005, 4., .005, .05);
    } else {
        set_spin_limits(m_depth, .1, 100., .1, 1.);    
    }
}

void SvgDialog::update_angle(bool use_deg) {
    if (m_use_deg == use_deg)
        return; // already setted
    m_use_deg = use_deg;

    m_rotation->set_unit(use_deg ? _u8L("°") : _u8L("rad"));
    if (use_deg) {
        set_limit_step(m_rotation, 180., 1.);
    } else {
        set_limit_step(m_rotation, M_PI, .02);
    }
}

void SvgDialog::set_depth(double depth_in_mm)
{
    m_depth->set_text(fmt::format("{:.10g}", depth_in_mm));
}

void SvgDialog::set_use_surface(bool checked)
{
    m_use_surface->set_checked(checked);
}

void SvgDialog::set_surface_distance(double distance_in_mm, double max_distance_in_mm)
{
    if (m_use_inch) {
        distance_in_mm *= MM_TO_INCH;
        max_distance_in_mm *= MM_TO_INCH;
    }

    // use N steps from zero to be able set to zero
    max_distance_in_mm = std::ceil(max_distance_in_mm / SURFACE_DISTANCE_STEP) * SURFACE_DISTANCE_STEP;

    m_surface_distance->set_begin_value(-max_distance_in_mm);
    m_surface_distance->set_end_value(max_distance_in_mm);
    m_surface_distance->set_value(distance_in_mm);
    m_surface_distance->set_default(0.); // To show revert button
}

void SvgDialog::set_rotation(double angle_in_rad)
{
    if (m_use_deg)
        angle_in_rad *= RAD_TO_DEG;    
    m_rotation->set_value(angle_in_rad);
    m_rotation->set_default(0.); // To show revert button
}

void SvgDialog::set_rotation_lock(bool lock)
{
    m_lock_rotation_btn->set_checked(lock);
}

namespace {
void enable_row_with_control(Item* control, bool enable)
{
    control->parent_item()->parent_item()->set_enabled(enable);
}
} // namespace

void SvgDialog::set_enable_use_surface(bool enable)
{
    enable_row_with_control(m_use_surface.get(), enable);
}

void SvgDialog::set_enable_surface_distance(bool enable)
{
    enable_row_with_control(m_surface_distance, enable);
}

} // namespace Slic3r::App::Plater
