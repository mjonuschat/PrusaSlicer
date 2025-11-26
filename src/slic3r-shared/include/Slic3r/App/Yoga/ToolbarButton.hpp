///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"

namespace Slic3r::App::Yoga {

class Toolbar;
class Dialog;

class ToolbarButton : public LayoutButton
{
public:
    ToolbarButton(Render::Icon icon, const std::string& tooltip = {});

    void render(Vec2f pos, Vec2f size) override;

    void style_node() override;

    Toolbar* get_subtoolbar() const;
    Toolbar* get_or_create_subtoolbar();
    Dialog* dialog() const;

private:
    Toolbar* m_subtoolbar = nullptr;
};

} // namespace Slic3r::App::Yoga
