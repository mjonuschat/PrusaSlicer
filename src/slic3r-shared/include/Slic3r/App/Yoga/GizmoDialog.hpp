///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/GizmoDialogHelp.hpp"

namespace Slic3r::App::Yoga {

class Icon;
class Text;
class LayoutButton;

class GizmoDialog : public Dialog
{
public:
    explicit GizmoDialog(const std::string& title);

protected:
    /*
     * @brief Add the separator into the specified item rather than into Dialog::context()
     */
    void add_separator(Item* item);

    float gap_size() const;
    Item* add_new_row(const std::string& title, Yoga::ItemPtr controls, YGAlign label_align = YGAlignCenter);

protected:
    GizmoDialogHelp m_help;

    Text* help_title(int help_item_idx);
    Icon* help_icon(int help_item_idx, int icon_idx = 0);

protected:

    struct HelpItem {
        std::vector<Icon*> icons;
        Text* title{ nullptr };
    };

    std::vector<HelpItem> m_help_items;
};

} // namespace Slic3r::App::Yoga
