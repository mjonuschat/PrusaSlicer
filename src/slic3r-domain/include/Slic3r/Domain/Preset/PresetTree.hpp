#pragma once

#include <optional>
#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Expr/ExprAst.hpp"

namespace Slic3r::Domain::Preset {

struct SourceLocation
{
    std::string file;
    size_t line{0};
    size_t column{0};

    std::string to_string() const
    {
        return file + " line:" + std::to_string(line) + " column: " + std::to_string(column);
    }
};

template <typename T>
struct SourceLocated
{
    T value;
    SourceLocation source_location;

    const T& operator*() const { return value; }
    T& operator*() { return value; }
};

using SourceLocatedExpr = SourceLocated<Expr::ExprAst>;

struct PresetNode
{
    std::string id;
    std::optional<std::string> name;
    std::vector<std::string> inherits;
    std::vector<std::string> unconditional_inherits;
    std::optional<SourceLocatedExpr> condition;
    PresetValueMap values;
    std::vector<PresetNode> variants;
};

struct RootPresetNode : PresetNode
{
    PresetKind kind{PresetKind::FdmPrinter};
};

using Presets = std::vector<RootPresetNode>;
using PresetCollection = EnumCollection<PresetKind, RootPresetNode>;

using PresetNodePath = std::vector<const PresetNode*>;
using NamedPresets = std::map<std::string, PresetNodePath>;
using NamedPresetsCollection = std::map<PresetKind, NamedPresets>;


bool is_public_name(const std::string& name);
std::string derive_name(const std::string& name, const std::string& parent_name);

} // namespace Slic3r::Domain::Presets
