#pragma once

#include "Slic3r/Domain/LayerHeightProfile.hpp"

#include <optional>
#include <variant>

namespace Slic3r::App::Undo {

struct CutGizmoState
{};

struct HeightRangeGizmoState
{
    std::optional<Domain::LayerHeightRange> selected_height_range;
};

using ToolState  = std::variant<CutGizmoState, HeightRangeGizmoState>;
using ToolsState = std::vector<ToolState>;

} // namespace Slic3r::App::Undo
