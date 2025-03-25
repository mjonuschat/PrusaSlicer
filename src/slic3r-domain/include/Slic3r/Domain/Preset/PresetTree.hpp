#pragma once

#include <optional>
#include "Slic3r/Domain/Preset/Types.hpp"


namespace Slic3r::Domain::Preset {

struct PresetNode
{
    std::string id;
    std::optional<std::string> name;
    std::vector<std::string> inherits;
    std::optional<std::string> condition;
    PresetValueMap values;
    std::vector<PresetNode> variants;
};

struct RootPresetNode : PresetNode
{
    PresetKind kind{PresetKind::FdmPrinter};
};

} // namespace Slic3r::Domain::Presets
