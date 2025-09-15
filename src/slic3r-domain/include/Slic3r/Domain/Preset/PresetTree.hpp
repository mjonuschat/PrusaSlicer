#pragma once

#include <optional>
#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Expr/ExprAst.hpp"
#include "Slic3r/Domain/Preset/SourceLocatedExpr.hpp"

namespace Slic3r::Domain::Preset {

enum class ConditionMatchMode
{
    FirstMatch,
    AllMatches
};

struct PresetNode
{
    std::string id;
    std::optional<std::string> name;
    std::vector<std::string> inherits;
    std::vector<std::string> unconditional_inherits;
    std::optional<SourceLocatedExpr> condition;
    std::optional<ConditionMatchMode> match_mode;
    PresetValueMap values;
    FeatureValueMap features;
    std::vector<PresetNode> variants;
    SourceLocation source_location;
};

struct RootPresetNode : PresetNode
{
    PresetKind kind{PresetKind::FdmPrinter};
};

using Presets = std::vector<RootPresetNode>;
using PresetCollection = std::map<PresetKind, Presets>;

using PresetNodePath = std::vector<const PresetNode*>;
using NamedPresets = std::map<std::string, PresetNodePath>;
using NamedPresetsCollection = std::map<PresetKind, NamedPresets>;


bool is_public_name(const std::string& name);
std::string derive_name(const std::string& name, const std::string& parent_name);

} // namespace Slic3r::Domain::Presets
