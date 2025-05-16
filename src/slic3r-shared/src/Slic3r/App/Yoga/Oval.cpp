#include "Slic3r/App/Yoga/Oval.hpp"

namespace Slic3r::App::Yoga {

Oval::Oval() : Rectangle() {}

void Oval::render(Vec2f pos, Vec2f size)
{
    set_rounding(0.5f * std::min(size.x(), size.y()));
    Rectangle::render(pos, size);
}

} // namespace Slic3r::App::Yoga