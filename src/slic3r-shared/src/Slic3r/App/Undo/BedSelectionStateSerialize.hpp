#pragma once

#include "Slic3r/App/Undo/SerializedData.hpp"
#include "Slic3r/App/Undo/BedSelectionState.hpp"

namespace Slic3r::App::Undo {

SerializedData serialize_bed_selection_state(const BedSelectionState& value);

BedSelectionState load_serialized_bed_selection_state(const SerializedData& data);

} // namespace Slic3r::App::Undo
