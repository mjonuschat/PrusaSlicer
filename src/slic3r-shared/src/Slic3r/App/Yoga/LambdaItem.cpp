#include "Slic3r/App/Yoga/LambdaItem.hpp"

namespace Slic3r::App::Yoga {

LambdaItem::LambdaItem(RenderPosFn render_fn, Item* parent) : Item(parent), m_render_fn(render_fn) {}

void LambdaItem::render(Vec2f pos, Vec2f size) {
    ImGui::SetCursorScreenPos(to_im(pos));
    m_render_fn(pos, size);
}

} // namespace Slic3r::App::Yoga
