#pragma once

#include <optional>
#include <vector>
#include <set>

#include <cereal/types/base_class.hpp>

#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Preset/SourceLocatedExpr.hpp"

namespace Slic3r::Domain::Preset {

enum class ConditionMatchMode
{
    FirstMatch,
    AllMatches
};

enum class PresetOrigin : uint8_t
{
    System,
    User
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

    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(id, name, inherits, unconditional_inherits, condition, match_mode, values, features, variants, source_location);
    }
};

struct RootPresetNode : PresetNode
{
    PresetKind kind{PresetKind::FdmPrinter};
    PresetOrigin origin{PresetOrigin::System};
    std::optional<std::string> user_file;
};

using Presets = std::vector<RootPresetNode>;
using PresetCollection = std::map<PresetKind, Presets>;

using PresetNodePath = std::vector<const PresetNode*>;
using IdentifiedPresets = std::map<std::string, PresetNodePath>;
using IdentifiedPresetsCollection = std::map<PresetKind, IdentifiedPresets>;

struct PresetName
{
    std::string name;
    std::set<std::string> id;
    PresetOrigin origin;

    template<class Archive> void
    serialize(Archive& archive)
    {
        std::vector<std::string> id_vec{id.begin(), id.end()};
        archive(name, id_vec, origin);
        id = std::set<std::string>{id_vec.begin(), id_vec.end()};
    }
};

using PresetNames = std::vector<PresetName>;
using PresetNamesCollection = std::map<PresetKind, PresetNames>;


bool is_public_name(const std::string& name);
std::string derive_name(const std::string& name, const std::string& parent_name);

} // namespace Slic3r::Domain::Presets
