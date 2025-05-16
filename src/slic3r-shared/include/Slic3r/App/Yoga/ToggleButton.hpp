///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <string>

namespace Slic3r::App::Yoga {

class Toggler;
class Text;

class ToggleButton : public AbstractButton
{
public:
    explicit ToggleButton(const std::string& label, const std::string& tooltip = {});

    void process_events(Vec2f pos, Vec2f size) override;

    void set_label(const std::string& label);
    std::string get_label();
    void set_font_type(Render::ImguiFontType font_type);

    void align_text(bool right_align);

private:
    Toggler*    m_toggler   { nullptr };
    Text*       m_label     { nullptr };
};

} // namespace Slic3r::App::Yoga
