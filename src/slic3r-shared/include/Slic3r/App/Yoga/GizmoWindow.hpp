///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/GizmoDialogHelp.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App::Yoga {

class Icon;
class Text;
class LayoutButton;
class Separator;

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
};
using GizmoWindowPtr = std::unique_ptr<GizmoWindow>;

} // namespace Slic3r::App::Yoga
