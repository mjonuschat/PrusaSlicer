///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {

Separator::Separator(Orientation orientation) : Rectangle()
{
    // We are abusing orientation attribute from Item, it doesnt matter
    // as we do not expect separator to actully have any children
    set_orientation(orientation);
    orientation == Orientation::Horizontal ? set_height(1) : set_width(1);
    set_max_size(
        orientation == Orientation::Horizontal ? Vec2f{YGUndefined, 1} : Vec2f{1, YGUndefined}
    );
    set_fill(ImColor(41, 41, 41));
    set_rounding(0);
}

Vec2f Separator::get_item_size()
{
    return orientation() == Orientation::Horizontal ? Vec2f{0, 1} : Vec2f{1, 0};
}

} // namespace Slic3r::App::Yoga
