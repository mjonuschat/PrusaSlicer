#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"

namespace Slic3r::Biz::Preset {

bool expr_string_equals(const Domain::Preset::Expressions& lhs, const Domain::Preset::Expressions& rhs);

class PresetCollectionEvaluator;

class PresetEvaluator
{
public:
    using HwPrinterConfig = Domain::Preset::HwPrinterConfig;
    using PresetKind = Domain::Preset::PresetKind;
    using Address = Domain::Preset::Address;
    using EvaluatedPrinterPreset = Domain::Preset::EvaluatedPrinterPreset;
    using EvaluatedPrinterPresets = std::vector<EvaluatedPrinterPreset>;

    explicit PresetEvaluator(const Domain::Preset::PresetCollection& presets)
        : m_presets(presets)
    {
        build_named_presets();
    }

    EvaluatedPrinterPresets evaluate(const HwPrinterConfig& hw_config) const;

private:
    friend class PresetCollectionEvaluator;

    using PresetNode = Domain::Preset::PresetNode;
    using PresetNodePath = Domain::Preset::PresetNodePath;
    using NamedPresets = Domain::Preset::NamedPresets;
    using NamedPresetsCollection = Domain::Preset::NamedPresetsCollection;
    using SourceLocation = Domain::Preset::SourceLocation;

    struct EvalPresetContext
    {
        std::string root_id;
        std::string id;
        std::string name;
        std::optional<Domain::Preset::ConditionMatchMode> match_mode{
            Domain::Preset::ConditionMatchMode::FirstMatch
        };
        Domain::Preset::Expressions conditions;
        Domain::Preset::PresetValueMap values;
        Domain::Preset::FeatureValueMap features;
        SourceLocation last_node_location;

        bool has_same_values(const EvalPresetContext& rhs) const;
    };

    using EvalPresetContexts = std::vector<EvalPresetContext>;

    void build_named_presets();
    void collect_named_presets(PresetKind kind, const PresetNode& node, const PresetNodePath& node_path);
    const PresetNode* find_node(PresetKind kind, std::string_view name) const;

    template <typename FdmConfigType, typename SlaConfigType>
    static Domain::Preset::EvaluatedPreset<FdmConfigType, SlaConfigType> preset_from_context(
        const Domain::Preset::HwPrinterConfig& hw_config,
        Domain::Preset::PresetKind kind, const EvalPresetContext& context
    );

    static EvalPresetContexts merged_same_presets(const EvalPresetContexts& presets);

private:
    const Domain::Preset::PresetCollection& m_presets;
    NamedPresetsCollection m_named_presets;
    Expr::Eval m_eval;
};


}