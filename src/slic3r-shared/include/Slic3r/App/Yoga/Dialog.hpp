///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AttachedWindow.hpp"

namespace Slic3r::App::Yoga {

class Text;
class LayoutButton;

class Dialog : public AttachedWindow {
public:
    explicit Dialog(const std::string title);

    bool closable() const;
    void set_closable(bool closable);

protected:
    Item* content() const;

    void add_separator();

private:
    bool m_closable = true;

    Text* m_title = nullptr;
    LayoutButton* m_close_button = nullptr;
    Item* m_content = nullptr;

    ImColor m_color_bg = ImColor(27, 27, 27);
    ImColor m_color_bg_alternate = ImColor(41, 41, 41);
};

}
