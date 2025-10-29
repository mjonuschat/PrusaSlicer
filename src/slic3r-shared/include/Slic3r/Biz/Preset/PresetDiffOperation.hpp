#pragma once

namespace Slic3r::Biz::Preset {
enum class PresetDiffOperation
{
    Undef, // used for Cross or Cancel buttons
    Transfer,
    Discard,
    Save,
};
} // namespace Slic3r::Biz::Preset
