///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Icon : public Item {
public:

    Icon(wchar_t icon, Item* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;

    wchar_t icon() const;
    void set_icon(wchar_t icon);

protected:
    Vec2f get_item_size() override;

private:
    wchar_t m_icon = '\0';
};

}
