#pragma once


#include <vector>
#include <set>
#include <string>
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3r::Domain::Preset {

using Expressions = std::vector<Expr::ExprAst>;

template <typename T>
concept ConfigBoxLike = std::is_base_of_v<ConfigBox, T>;

template <typename T>
concept ConfigBoxLikeOrMonostate = std::is_base_of_v<ConfigBox, T>
    || std::is_same_v<std::monostate, T>;

struct EvaluatedPresetMetadata
{
    std::string root_id;
    std::string id;
    std::string name;
    Expressions conditions;
};

template <ConfigBoxLike ConfigFdmType, ConfigBoxLikeOrMonostate ConfigSlaType>
struct EvaluatedPreset
{
    using PresetValues = std::variant<ConfigFdmType, ConfigSlaType>;

    PresetKind kind{PresetKind::FdmPrinter};
    std::string root_id;
    std::string id;
    std::string name;
    PresetValues values;
    FeatureValueMap features;
    Expressions conditions;
    SourceLocation last_node_location;

    static EvaluatedPreset make(PresetKind kind, const EvaluatedPresetMetadata& metadata, PresetValues values)
    {
        return {
            .kind = kind,
            .root_id = metadata.root_id,
            .id = metadata.id,
            .name = metadata.name,
            .values = values,
            .conditions = metadata.conditions,
        };
    }

    [[nodiscard]] EvaluatedPresetMetadata metadata() const
    {
        return {
            .root_id = root_id,
            .id = id,
            .name = name,
            .conditions = conditions,
        };
    }

    [[nodiscard]] std::string_view short_name() const
    {
        size_t idx = name.find('@');
        if (idx == 0 || idx == std::string_view::npos)
            return name;
        while (idx > 0 && name[idx] != ' ') idx--;
        return std::string_view{name.data(), idx};
    }

    [[nodiscard]] const ConfigBox& config_box() const
    {
        return std::visit(
            overloaded{
                [](const auto& c) -> const ConfigBox& { return c; },
                [](const std::monostate& c) -> const ConfigBox& { PANIC("Unsupported"); },
            },
            values
        );
    }

    [[nodiscard]] ConfigBox& config_box()
    {
        return std::visit(
            overloaded{
                [](auto& c) -> ConfigBox& { return c; },
                [](std::monostate& c) -> ConfigBox& { PANIC("Unsupported"); },
            },
            values
        );
    }
};

// using EvaluatedPresets = std::vector<EvaluatedPreset>;

struct EvaluatedToolPrintPreset
{
    using Preset = EvaluatedPreset<ToolPrintSettings, std::monostate>;
    Preset preset;

    EvaluatedToolPrintPreset()                                    = default;
    EvaluatedToolPrintPreset(const EvaluatedToolPrintPreset&)     = default;
    EvaluatedToolPrintPreset(EvaluatedToolPrintPreset&&) noexcept = default;

    explicit EvaluatedToolPrintPreset(Preset&& preset) : preset(std::move(preset)) {}
};

/**
 * @brief Single tool `tool_print` presets variants (e.g. quality vs. speed variants).
 */
using SingleToolEvaluatedToolPrintPresets = std::vector<EvaluatedToolPrintPreset>;
/**
 * @brief `tool_print` variants for all tools.
 */
using AllToolsEvaluatedToolPrintPresets = std::vector<SingleToolEvaluatedToolPrintPresets>;

struct EvaluatedMaterialPreset
{
    using Preset = EvaluatedPreset<FilamentSettings, SLAMaterialSettings>;
    Preset preset;

    EvaluatedMaterialPreset()                                   = default;
    EvaluatedMaterialPreset(const EvaluatedMaterialPreset&)     = default;
    EvaluatedMaterialPreset(EvaluatedMaterialPreset&&) noexcept = default;

    explicit EvaluatedMaterialPreset(Preset&& preset) : preset(std::move(preset)) {}
};

using SingleToolEvaluatedMaterialPresets = std::vector<EvaluatedMaterialPreset>;
using AllToolsEvaluatedMaterialPresets  = std::vector<SingleToolEvaluatedMaterialPresets>;


struct EvaluatedPrintPreset
{
    using Preset = EvaluatedPreset<PrintSettings, SLAPrintSettings>;
    Preset preset;
    AllToolsEvaluatedToolPrintPresets tools;
    AllToolsEvaluatedMaterialPresets materials;

    EvaluatedPrintPreset()                                = default;
    EvaluatedPrintPreset(const EvaluatedPrintPreset&)     = default;
    EvaluatedPrintPreset(EvaluatedPrintPreset&&) noexcept = default;

    EvaluatedPrintPreset(
        Preset&& preset,
        AllToolsEvaluatedToolPrintPresets&& tools,
        AllToolsEvaluatedMaterialPresets&& materials
    ) :
        preset(std::move(preset)),
        tools(std::move(tools)),
        materials(std::move(materials))
    {}

    const EvaluatedToolPrintPreset* find_tool_preset_by_id(size_t tool_idx, const std::string& id) const;
    const EvaluatedMaterialPreset* find_material_preset_by_id(size_t tool_idx, const std::string& id) const;
};

using EvaluatedPrintPresets = std::vector<EvaluatedPrintPreset>;


struct EvaluatedPrinterPreset
{
    using Preset = EvaluatedPreset<PrinterSettings, SLAPrinterSettings>;
    HwPrinterConfig hw_config;
    Preset preset;
    EvaluatedPrintPresets prints;

    const EvaluatedPrintPreset* find_print_preset_by_id(const std::string& id) const;

    PrinterTechnology technology() const
    {
        return preset.kind == PresetKind::FdmPrinter ? PrinterTechnology::FFF :
                                                       PrinterTechnology::SLA;
    }

    bool is_valid() const;
};

} // namespace Slic3r::Domain::Preset
