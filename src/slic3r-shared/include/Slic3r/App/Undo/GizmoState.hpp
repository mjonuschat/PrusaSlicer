#pragma once

#include "Slic3r/Domain/LayerHeightProfile.hpp"

#include <optional>

namespace Slic3r::App::Undo {

struct GizmoState
{
    std::optional<Domain::LayerHeightRange> selected_height_range;
};

} // namespace Slic3r::App::Undo
