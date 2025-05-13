///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

class Toolbar;

class ToolbarButton : public AbstractButton
{
public:
    explicit ToolbarButton(wchar_t icon, const std::string& tooltip = {}, Toolbar* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;

    void style_node() override;

    void set_subtoolbar_buttons(const std::vector<ToolbarButton*>& buttons);

    Toolbar* subtoolbar() const;

private:
    Toolbar* m_subtoolbar = nullptr;
};

} // namespace Slic3r::App::Yoga
