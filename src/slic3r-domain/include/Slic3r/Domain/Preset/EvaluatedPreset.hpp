#pragma once

#include <vector>
#include <set>
#include <string>
#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace Slic3r::Domain::Preset {

struct EvaluatedPreset
{
    using Expressions = std::vector<Expr::ExprAst>;

    PresetKind kind{PresetKind::FdmPrinter};
    std::string id;
    std::string name;
    PresetValueMap values;
    FeatureValueMap features;
    Expressions conditions;
    SourceLocation last_node_location;
};

using EvaluatedPresets = std::vector<EvaluatedPreset>;

struct EvaluatedToolPrintPreset
{
    EvaluatedPreset preset;

    EvaluatedToolPrintPreset() = default;
    EvaluatedToolPrintPreset(const EvaluatedToolPrintPreset&) = default;
    EvaluatedToolPrintPreset(EvaluatedToolPrintPreset&&) = default;

    explicit EvaluatedToolPrintPreset(EvaluatedPreset&& preset)
        : preset(std::move(preset))
    {}
};

/**
 * @brief Single tool `tool_print` presets variants (e.g. quiality vs. speed variants).
 */
using EvaluatedToolPrintPresetVariants = std::vector<EvaluatedToolPrintPreset>;
/**
 * @brief `tool_print` variants for all tools.
 */
using EvaluatedToolPrintPresets = std::vector<EvaluatedToolPrintPresetVariants>;

struct EvaluatedPrintPreset
{
    EvaluatedPreset preset;
    EvaluatedToolPrintPresets tools;

    EvaluatedPrintPreset() = default;
    EvaluatedPrintPreset(const EvaluatedPrintPreset&) = default;
    EvaluatedPrintPreset(EvaluatedPrintPreset&&) = default;

    EvaluatedPrintPreset(EvaluatedPreset&& preset, EvaluatedToolPrintPresets&& tools)
        : preset(std::move(preset)), tools(std::move(tools))
    {}
};

using EvaluatedPrintPresets = std::vector<EvaluatedPrintPreset>;

struct EvaluatedMaterialPreset
{
    EvaluatedPreset preset;
    std::set<std::string> incompatible_tool_print_ids;

    EvaluatedMaterialPreset() = default;
    EvaluatedMaterialPreset(const EvaluatedMaterialPreset&) = default;
    EvaluatedMaterialPreset(EvaluatedMaterialPreset&&) = default;

    explicit EvaluatedMaterialPreset(EvaluatedPreset&& preset)
        : preset(std::move(preset))
    {}
};

using EvaluatedMaterialPresets = std::vector<EvaluatedMaterialPreset>;

struct EvaluatedPrinterPreset
{
    EvaluatedPreset preset;
    EvaluatedPrintPresets prints;
    EvaluatedMaterialPresets materials;
};

}
