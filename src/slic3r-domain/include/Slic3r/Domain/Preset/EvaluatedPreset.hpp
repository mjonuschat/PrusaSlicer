#pragma once

#include <vector>
#include <set>
#include <string>
#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace Slic3r::Domain::Preset {

struct EvaluatedPreset
{
    using Expressions = std::vector<Expr::ExprAst>;

    std::string id;
    std::string name;
    PresetKind kind{PresetKind::FdmPrinter};
    PresetValueMap values;
    Expressions conditions;
};

using EvaluatedPresets = std::vector<EvaluatedPreset>;

struct EvaluatedToolPrintPreset
{
    EvaluatedPreset preset;
};

using EvaluatedToolPrintPresets = std::vector<EvaluatedToolPrintPreset>;

struct EvaluatedPrintPreset
{
    EvaluatedPreset preset;
    EvaluatedToolPrintPresets tools;
};

using EvaluatedPrintPresets = std::vector<EvaluatedPrintPreset>;

struct EvaluatedMaterialPreset
{
    EvaluatedPreset preset;
    std::set<std::string> incompatible_tool_print_ids;
};

using EvaluatedMaterialPresets = std::vector<EvaluatedMaterialPreset>;

struct EvaluatedPrinterPreset
{
    EvaluatedPreset preset;
    EvaluatedPrintPresets prints;
    EvaluatedMaterialPresets materials;
};

}
