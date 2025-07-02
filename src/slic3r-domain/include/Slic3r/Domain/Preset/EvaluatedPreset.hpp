#pragma once

#include <vector>
#include <set>
#include <string>
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"

namespace Slic3r::Domain::Preset {


using Expressions = std::vector<Expr::ExprAst>;

template <typename ConfigFdmType, typename ConfigSlaType>
struct EvaluatedPreset
{
    using PresetValues = std::variant<ConfigFdmType, ConfigSlaType>;

    PresetKind kind{PresetKind::FdmPrinter};
    std::string id;
    std::string name;
    PresetValues values;
    FeatureValueMap features;
    Expressions conditions;
    SourceLocation last_node_location;
};

//using EvaluatedPresets = std::vector<EvaluatedPreset>;

struct EvaluatedToolPrintPreset
{
    using Presets = EvaluatedPreset<ToolPrintSettings, std::monostate>;
    Presets preset;

    EvaluatedToolPrintPreset() = default;
    EvaluatedToolPrintPreset(const EvaluatedToolPrintPreset&) = default;
    EvaluatedToolPrintPreset(EvaluatedToolPrintPreset&&) = default;

    explicit EvaluatedToolPrintPreset(Presets&& preset)
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
    using Presets = EvaluatedPreset<PrintSettings, SLAPrintSettings>;
    Presets preset;
    EvaluatedToolPrintPresets tools;

    EvaluatedPrintPreset() = default;
    EvaluatedPrintPreset(const EvaluatedPrintPreset&) = default;
    EvaluatedPrintPreset(EvaluatedPrintPreset&&) = default;

    EvaluatedPrintPreset(Presets&& preset, EvaluatedToolPrintPresets&& tools)
        : preset(std::move(preset)), tools(std::move(tools))
    {}
};

using EvaluatedPrintPresets = std::vector<EvaluatedPrintPreset>;

struct EvaluatedMaterialPreset
{
    using Presets = EvaluatedPreset<FilamentSettings, SLAMaterialSettings>;
    Presets preset;
    std::set<std::string> incompatible_tool_print_ids;

    EvaluatedMaterialPreset() = default;
    EvaluatedMaterialPreset(const EvaluatedMaterialPreset&) = default;
    EvaluatedMaterialPreset(EvaluatedMaterialPreset&&) = default;

    explicit EvaluatedMaterialPreset(Presets&& preset)
        : preset(std::move(preset))
    {}
};

using EvaluatedMaterialPresets = std::vector<EvaluatedMaterialPreset>;

struct EvaluatedPrinterPreset
{
    using Presets = EvaluatedPreset<PrinterSettings, SLAPrinterSettings>;
    Presets preset;
    EvaluatedPrintPresets prints;
    EvaluatedMaterialPresets materials;
};

}
