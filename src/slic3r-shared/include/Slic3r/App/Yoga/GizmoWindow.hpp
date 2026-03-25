///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/GizmoDialogHelp.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App::Yoga {

class Item;
class Icon;
class Text;
class LayoutButton;
class Separator;
class InputTextWithSpin;
class SliderWithInput;

class GizmoWindow : public Window
{
public:
    struct GizmoCallbacks
    {
        std::function<void()> close_requested{nullptr};
        std::function<void()> revert_requested{nullptr};
    };

    explicit GizmoWindow(const std::string& title, Render::Icon icon);

    GizmoCallbacks& gizmo_callbacks();

protected:
    explicit GizmoWindow();

    /*
     * @brief Add the separator into the specified item rather than into Dialog::context()
     */
    Separator* add_separator(Item* item);

    float gap_size() const;
    Item* add_new_row(
        const std::string& title,
        Yoga::ItemPtr controls,
        YGAlign label_align = YGAlignCenter
    );

    Item* content() const;

    LayoutButton* close_button() const;
    LayoutButton* revert_button() const;

    /**
     * @brief Adds a revert button to the given parent item.
     *
     * @param parent  Parent UI item.
     * @param tooltip Tooltip text for the revert button.
     * @return Pointer to the created LayoutButton.
     */
    LayoutButton* add_revert_btn(Item* parent, const std::string& tooltip);

    /**
     * @brief Adds a wrapper item with flex-shrink enabled.
     *
     * @param parent Parent UI item.
     * @return Pointer to the created Item.
     */
    Item* add_flex_shrinked_wrap(Item* parent);

    /**
     * @brief Creates a row item with a bold text label.
     *
     * @param parent Parent UI item.
     * @param label  Text label displayed in the row.
     * @return Pointer to the created row Item.
     */
    Item* add_labeled_row(Item* parent, const std::string& label);

    /**
     * @brief Adds a row with an integer spin input.
     *
     * Optionally adds a revert button if @p revert_button_tooltip is not empty.
     *
     * @param title                   Row title.
     * @param parent                  Parent UI item.
     * @param input                   Output pointer to the created input control.
     * @param unit                    Unit label displayed next to the input.
     * @param revert_button_tooltip   Tooltip for the revert button (if empty, button is not created).
     * @param min                     Minimum allowed value.
     * @param max                     Maximum allowed value.
     * @return Pointer to the created row Item.
     */
    Item* add_row_with_spin_int(
        const std::string& title,
        Yoga::Item* parent,
        Yoga::InputTextWithSpin** input,
        const std::string& unit,
        const std::string& revert_button_tooltip,
        int min,
        int max
    );

    /**
     * @brief Adds a row with a double-precision spin input.
     *
     * Optionally adds a revert button if @p revert_button_tooltip is not empty.
     *
     * @param title                   Row title.
     * @param parent                  Parent UI item.
     * @param input                   Output pointer to the created input control.
     * @param unit                    Unit label displayed next to the input.
     * @param revert_button_tooltip   Tooltip for the revert button (if empty, button is not created).
     * @param min                     Minimum allowed value.
     * @param max                     Maximum allowed value.
     * @param step                    Increment step.
     * @param step_fast               Fast increment step.
     * @return Pointer to the created row Item.
     */
    Item* add_row_with_spin_double(
        const std::string& title,
        Yoga::Item* parent,
        Yoga::InputTextWithSpin** input,
        const std::string& unit,
        const std::string& revert_button_tooltip,
        double min,
        double max,
        double step,
        double step_fast
    );

    /**
     * @brief Adds a row with a slider and input field.
     *
     * Optionally adds a revert button if @p revert_tooltip is not empty.
     *
     * @param parent         Parent UI item.
     * @param slider         Output pointer to the created slider control.
     * @param name           Name/label of the slider.
     * @param unit           Unit label displayed next to the slider.
     * @param revert_tooltip Tooltip for the revert button (if empty, button is not created).
     * @return Pointer to the created row Item.
     */
    Item* add_row_with_slider(
        Item* parent,
        SliderWithInput** slider,
        const std::string& name,
        const std::string& unit,
        const std::string& revert_tooltip
    );

protected:
    GizmoCallbacks m_gizmo_callback;

    GizmoDialogHelp m_help;

    LayoutButton* m_close_button  = nullptr;
    LayoutButton* m_revert_button = nullptr;
    Item* m_content               = nullptr;
    Item* m_top_row               = nullptr;
    Item* m_tab_container         = nullptr;
    size_t m_current_tab_index    = 0;

    ImColor m_color_bg           = ImColor(27, 27, 27);
    ImColor m_color_bg_alternate = ImColor(41, 41, 41);
    std::vector<LayoutButton*> m_tab_buttons;
    ButtonGroup m_tab_button_group;

    float m_label_width = 85.f;
};

using GizmoWindowPtr = std::unique_ptr<GizmoWindow>;

} // namespace Slic3r::App::Yoga
