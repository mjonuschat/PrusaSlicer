#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Plater {

enum class SelectedColor
{
    None,
    Primary,
    Secondary,
    Both
};

Yoga::Item* emplace_icon(
    Yoga::Item* parent,
    Render::Icon icon,
    const ImVec2& size,
    ImColor color = {255, 255, 255}
);

Yoga::Item* append(Yoga::Item* parent, Yoga::ItemPtr item);

} // namespace Slic3r::App::Plater
