#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"

namespace Slic3r::Biz::Preset {

class PresetCollectionEvaluator;

class PresetEvaluator
{
public:
    using HwPrinterConfig = Domain::Preset::HwPrinterConfig;
    using EvaluatedPresets = Domain::Preset::EvaluatedPresets;
    using PresetKind = Domain::Preset::PresetKind;
    using Address = Domain::Preset::Address;
    using EvaluatedPrinterPreset = Domain::Preset::EvaluatedPrinterPreset;

    explicit PresetEvaluator(const Domain::Preset::PresetCollection& presets)
        : m_presets(presets)
    {
        build_named_presets();
    }

    EvaluatedPrinterPreset evaluate(PresetKind kind, const HwPrinterConfig& hw_config) const;

private:
    friend class PresetCollectionEvaluator;

    using PresetNode = Domain::Preset::PresetNode;
    using PresetNodePath = Domain::Preset::PresetNodePath;
    using NamedPresets = Domain::Preset::NamedPresets;
    using NamedPresetsCollection = Domain::Preset::NamedPresetsCollection;
    using EvaluatedPreset = Domain::Preset::EvaluatedPreset;

    struct EvalPresetContext
    {
        std::string id;
        std::string name;
        EvaluatedPreset::Expressions conditions;
        Domain::Preset::PresetValueMap values;
    };

    using EvalPresetContexts = std::vector<EvalPresetContext>;

    void build_named_presets();
    void collect_named_presets(PresetKind kind, const PresetNode& node, const PresetNodePath& node_path);
    const PresetNode* find_node(PresetKind kind, std::string_view name) const;

    EvalPresetContexts eval_presets(PresetKind kind, const PresetNode& node, const Expr::ValueMap& vars) const;

private:
    const Domain::Preset::PresetCollection& m_presets;
    NamedPresetsCollection m_named_presets;
    Expr::Eval m_eval;
};


}