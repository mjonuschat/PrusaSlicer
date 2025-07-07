///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Text;
class LayoutButton;

class GizmoDialog : public Dialog {
public:
    explicit GizmoDialog(const std::string& title);

    struct HelpIcon {
        Render::Icon    icon        {Render::Icon::None};
        Vec2f           min_size    {25.f, 25.f};
    };

protected:
    /*
    * @brief Add the separator into the specified item rather than into Dialog::context()
    */
    void add_separator(Item* item);

    Item* add_help(const std::vector<HelpIcon> symbols,
                   const std::string title,
                   Item* help_container,
                   bool is_grayed = true);
};

}
