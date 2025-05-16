///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class LambdaItem : public Item {
public:
    LambdaItem(RenderPosFn render_fn);

    void render(Vec2f pos, Vec2f size) override;

private:
    RenderPosFn m_render_fn;
};

}
