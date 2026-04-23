#include "Slic3r/App/Plater/MMPaintingUtils.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"

namespace Slic3r::App::Plater {

Yoga::Item* emplace_icon(
    Yoga::Item* parent,
    Render::Icon icon,
    const ImVec2& size,
    ImColor color
)
{
    Yoga::Icon* result{parent->emplace_back<Yoga::Icon>(icon)};
    result->set_width(size.x);
    result->set_height(size.y);
    result->set_fill_mode(Yoga::Icon::FillMode::PreservedAspectCentered);
    result->set_tint(color);
    return result;
}

Yoga::Item* append(Yoga::Item* parent, Yoga::ItemPtr item)
{
    auto item_ptr{item.get()};
    parent->append(std::move(item));
    return item_ptr;
}

} // namespace Slic3r::App::Plater
