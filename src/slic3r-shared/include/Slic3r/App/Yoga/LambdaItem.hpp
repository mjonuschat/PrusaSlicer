#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class LambdaItem : public Item {
public:
    LambdaItem(RenderPosFn render_fn, Item* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;

private:
    RenderPosFn m_render_fn;
};

}
