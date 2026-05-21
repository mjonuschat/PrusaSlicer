///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/GizmoWindow.hpp"

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Plater/WarningPanel.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

constexpr float dialog_padding = 20;

GizmoWindow::GizmoWindow(const std::string& title, Render::Icon icon, const std::string& shortcut) :
    Window("GizmoWindow")
{
    set_orientation(Orientation::Horizontal);
    set_gap(0);
    set_padding(0);
    set_flex_grow(1);
    set_flags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    Item* column = emplace_back<Item>();
    column->set_orientation(Orientation::Vertical);
    column->set_flex_grow(1);

    Item* top_row = column->emplace_back<Item>();
    top_row->set_max_size({YGUndefined, 40});
    top_row->set_flex_shrink(0);

    Rectangle* buttons_rect = top_row->emplace_back<Rectangle>();
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(dialog_padding);
    buttons_rect->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
    buttons_rect->set_flex_grow(1);
    buttons_rect->set_flags(ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);

    if (icon != Render::Icon::None) {
        Icon* header_icon = buttons_rect->emplace_back<Icon>(icon);
        header_icon->set_margin(Margins{0, 0, 3, 0});
        header_icon->set_width(20);
        header_icon->set_height(20);
        header_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        header_icon->set_flex_shrink(0.f);
    } else {
        buttons_rect->set_padding(Paddings{24, dialog_padding, dialog_padding, dialog_padding});
    }

    Text* title_text = buttons_rect->emplace_back<Text>(title);
    title_text->set_font_type(Render::ImguiFontType::Bold);

    if (!shortcut.empty()) {
        Text* shortcut_text = buttons_rect->emplace_back<Text>(shortcut);
        shortcut_text->set_margin({6, 0, 0, 0});
        shortcut_text->set_text_color(
            m_theme->color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled)
        );
    }

    Item* spacer = buttons_rect->emplace_back<Item>();
    spacer->set_flex_grow(1);

    m_revert_button =
        buttons_rect->emplace_back<LayoutButton>(std::string{}, Render::Icon::UndoGizmo);
    m_revert_button->set_width(24);
    m_revert_button->set_height(24);
    m_revert_button->callbacks().action = [this]
    {
        if (m_gizmo_callback.revert_requested) {
            m_gizmo_callback.revert_requested();
        }
    };
    m_revert_button->set_visible(false);
    m_revert_button->set_margin(Margins(10.f, 0.f));

    m_close_button =
        buttons_rect->emplace_back<LayoutButton>(std::string{}, Render::Icon::PrintIdle);
    m_close_button->set_width(20);
    m_close_button->set_height(20);
    m_close_button->callbacks().action = [this]
    {
        if (m_gizmo_callback.close_requested) {
            m_gizmo_callback.close_requested();
        }
    };

    m_top_bar = column->emplace_back<Item>();

    m_content = column->emplace_back<ScrollArea>();
    m_content->set_padding(
        Paddings{dialog_padding, dialog_padding * 0.5, dialog_padding, dialog_padding * 0.5}
    );
    m_content->set_flex_grow(1);
    m_content->set_gap(gap_size());
    m_content->set_orientation(Orientation::Vertical);

    m_warning_panel = column->emplace_back<WarningPanel>();
    m_warning_panel->set_flex_shrink(0);
    m_warning_panel->set_visible(false);

    m_bottom_bar = column->emplace_back<Item>();
}

GizmoWindow::GizmoCallbacks& GizmoWindow::gizmo_callbacks()
{
    return m_gizmo_callback;
}

Separator* GizmoWindow::add_separator(Item* item)
{
    Separator* separator = item->emplace_back<Separator>(Orientation::Horizontal);

    float margin_begin{0.f};
    float margin_end{0.f};
    Item* parent_item = item;
    while (parent_item != this) {
        margin_begin += item->orientation() == Orientation::Vertical ? parent_item->padding().left :
                                                                       parent_item->padding().top;
        margin_end += item->orientation() == Orientation::Vertical ? parent_item->padding().right :
                                                                     parent_item->padding().bottom;
        parent_item = parent_item->parent_item();
    }

    if (item->orientation() == Orientation::Vertical) {
        separator->set_margin(Margins(-margin_begin, 0.f, -margin_end, 0.f));
    } else {
        separator->set_margin(Margins(0.f, -margin_begin, 0.f, -margin_end));
    }
    return separator;
}

float GizmoWindow::gap_size() const
{
    return 10.f;
}

float GizmoWindow::preffered_max_width() const
{
    return 400.f;
}

Item*
GizmoWindow::add_new_row(const std::string& title, Yoga::ItemPtr controls, YGAlign label_align)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(gap_size());
    row->set_flex_shrink(0);
    Text* text = row->emplace_back<Text>(title);
    text->set_self_align(label_align);
    text->set_width(100);

    controls->set_flex_grow(1);
    row->append(std::move(controls));
    return row;
}

Item* GizmoWindow::content() const
{
    return m_content;
}

Item* GizmoWindow::top_bar() const
{
    return m_top_bar;
}

Item* GizmoWindow::bottom_bar() const
{
    return m_bottom_bar;
}

LayoutButton* GizmoWindow::close_button() const
{
    return m_close_button;
}

LayoutButton* GizmoWindow::revert_button() const
{
    return m_revert_button;
}

Item* GizmoWindow::add_flex_shrinked_wrap(Item* parent)
{
    Item* wrap = parent->emplace_back<Item>();
    wrap->set_flex_grow(1.f);
    wrap->set_flex_shrink(0.f);
    wrap->set_gap(3.f);
    return wrap;
}

Item* GizmoWindow::add_non_shrinked_wrap(Item* parent, Orientation orientation, float gap)
{
    Item* wrap = parent->emplace_back<Item>();
    wrap->set_orientation(orientation);
    wrap->set_flex_shrink(0.f);
    wrap->set_gap(gap);
    return wrap;
}

LayoutButton* GizmoWindow::add_revert_btn(Item* parent, const std::string& tooltip)
{
    Item* revert_space = parent->emplace_back<Item>();
    revert_space->set_min_size(Vec2f(24.f, 24.f));
    revert_space->set_justify_content(YGJustifyFlexEnd);
    LayoutButton* revert_btn =
        revert_space->emplace_back<LayoutButton>(std::string{}, Render::Icon::DSRevert, tooltip);
    revert_btn->set_min_size(Vec2f(20.f, 20.f));
    revert_btn->set_self_align(YGAlignCenter);
    return revert_btn;
}

Item* GizmoWindow::add_labeled_row(Item* parent, const std::string& label)
{
    Item* labeled_row = parent->emplace_back<Item>();
    labeled_row->set_gap(10);
    labeled_row->set_flex_shrink(0.f);

    Text* text = labeled_row->emplace_back<Text>(label);
    text->set_width(m_label_width);
    text->set_flex_shrink(0.f);
    text->set_font_type(Render::ImguiFontType::Bold);
    text->set_self_align(YGAlignCenter);

    return labeled_row;
}

Item* GizmoWindow::add_row_with_spin_int(
    const std::string& title,
    Yoga::Item* parent,
    Yoga::InputTextWithSpin** input,
    const std::string& unit,
    const std::string& revert_button_tooltip,
    int min,
    int max
)
{
    Item* row           = add_labeled_row(parent, title);
    Item* wrap_row_item = add_flex_shrinked_wrap(row);

    (*input) =
        wrap_row_item->emplace_back<InputTextWithSpin>(std::make_unique<IntValidator>(min, max));

    wrap_row_item->emplace_back<Text>(unit)->set_self_align(YGAlignCenter);

    if (!revert_button_tooltip.empty()) {
        (*input)->set_revert_button(add_revert_btn(wrap_row_item, revert_button_tooltip));
        (*input)->set_flex_grow(1.f);
    }

    return row;
}

Item* GizmoWindow::add_row_with_spin_double(
    const std::string& title,
    Yoga::Item* parent,
    Yoga::InputTextWithSpin** input,
    const std::string& unit,
    const std::string& revert_button_tooltip,
    double min,
    double max,
    double step,
    double step_fast
)
{
    Item* row           = add_labeled_row(parent, title);
    Item* wrap_row_item = add_flex_shrinked_wrap(row);

    (*input) = wrap_row_item->emplace_back<InputTextWithSpin>(
        std::make_unique<DoubleValidator>(min, max),
        step,
        step_fast
    );

    wrap_row_item->emplace_back<Text>(unit)->set_self_align(YGAlignCenter);

    if (!revert_button_tooltip.empty()) {
        (*input)->set_revert_button(add_revert_btn(wrap_row_item, revert_button_tooltip));
    }
    (*input)->set_flex_grow(1.f);

    return row;
}

Item* GizmoWindow::add_row_with_slider(
    Item* parent,
    SliderWithInput** slider,
    const std::string& name,
    const std::string& unit,
    const std::string& revert_tooltip
)
{
    Item* wrap = add_non_shrinked_wrap(parent, Orientation::Vertical, gap_size());

    Text* header = wrap->emplace_back<Text>(name);
    header->set_font_type(Render::ImguiFontType::Bold);
    header->set_margin(Margins(0, 5, 0, 0));

    Item* line_wrap = wrap->emplace_back<Item>();
    line_wrap->set_flex_shrink(0.f);

    (*slider) = line_wrap->emplace_back<SliderWithInput>(unit);

    (*slider)->set_flex_grow(1.f);
    if (!revert_tooltip.empty()) {
        (*slider)->set_revert_button(add_revert_btn(line_wrap, revert_tooltip));
    }
    return line_wrap;
}

Item* GizmoWindow::add_row_with_combo_box(
    const std::string& title,
    Item* parent,
    ComboBox** combo,
    const std::string& revert_tooltip
)
{
    Item* row           = add_labeled_row(parent, title);
    Item* wrap_row_item = add_flex_shrinked_wrap(row);

    (*combo) = wrap_row_item->emplace_back<ComboBox>(title);

    if (!revert_tooltip.empty()) {
        (*combo)->set_revert_button(add_revert_btn(wrap_row_item, revert_tooltip));
    }
    (*combo)->set_flex_grow(1.f);

    return row;
}

Item* GizmoWindow::add_row_with_toggle_button(
    const std::string& title,
    Item* parent,
    Yoga::ToggleButton** toggle,
    const std::string& revert_tooltip
)
{
    Item* row           = add_labeled_row(parent, title);
    Item* wrap_row_item = add_flex_shrinked_wrap(row);

    (*toggle) = wrap_row_item->emplace_back<ToggleButton>();

    if (!revert_tooltip.empty()) {
        (*toggle)->set_revert_button(add_revert_btn(wrap_row_item, revert_tooltip));
    }

    return row;
}

Item* GizmoWindow::add_row_with_button(
    Item* parent,
    LayoutButton** button,
    const std::string& label,
    const std::string& tooltip,
    Render::Icon icon
)
{
    Item* row = parent->emplace_back<Item>();
    row->set_gap(gap_size());
    row->set_flex_shrink(0.f);
    (*button) = row->emplace_back<LayoutButton>(label, icon, tooltip);
    (*button)->set_content_padding({10.f, 5.f});
    return row;
}

Yoga::LayoutButton*
GizmoWindow::add_icon_button(Item* parent, Render::Icon icon, const std::string& tooltip)
{
    LayoutButton* button = parent->emplace_back<LayoutButton>(std::string(), icon, tooltip);
    button->set_checkable(true);
    button->set_min_size({40.f, 40.f});
    button->set_content_padding(8.f);
    return button;
}

void GizmoWindow::set_warning(const std::string& title, const std::string& text)
{
    m_warning_panel->set_warning(title, text);
    m_warning_panel->set_visible(true);
}

void GizmoWindow::set_warning(const std::string& title, const std::vector<std::string>& errors)
{
    m_warning_panel->set_warning(title, errors);
    m_warning_panel->set_visible(true);
}

void GizmoWindow::clear_warning()
{
    m_warning_panel->set_visible(false);
}

} // namespace Slic3r::App::Plater
