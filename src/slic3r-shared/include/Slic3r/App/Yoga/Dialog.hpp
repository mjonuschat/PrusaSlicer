///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Popup.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App::Yoga {

class Text;
class LayoutButton;

class Dialog : public Popup
{
public:
    explicit Dialog(const std::string& tab);
    explicit Dialog(std::initializer_list<std::string> tabs);

    bool closable() const;
    void set_closable(bool closable);

    void set_current_tab(size_t current_index);

protected:
    Item* content() const;
    void add_separator();

    virtual void on_tab_selected(int current_index);

private:
    bool m_closable = true;

    LayoutButton* m_close_button = nullptr;
    Item* m_content = nullptr;

    ImColor m_color_bg = ImColor(27, 27, 27);
    ImColor m_color_bg_alternate = ImColor(41, 41, 41);
    std::vector<LayoutButton*> m_tab_buttons;
    ButtonGroup m_tab_button_group;
};

} // namespace Slic3r::App::Yoga
