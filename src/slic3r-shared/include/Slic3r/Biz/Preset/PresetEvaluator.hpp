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

    EvaluatedPrinterPresets evaluate(const HwPrinterConfig& hw_config, bool use_material_cache = true) const;

private:
    friend class PresetCollectionEvaluator;

    using PresetNode = Domain::Preset::PresetNode;
    using PresetNodePath = Domain::Preset::PresetNodePath;
    using IdentifiedPresets = Domain::Preset::IdentifiedPresets;
    using IdentifiedPresetsCollection = Domain::Preset::IdentifiedPresetsCollection;
    using SourceLocation = Domain::Preset::SourceLocation;

    struct EvalPresetContext
    {
        Domain::Preset::PresetOrigin origin{Domain::Preset::PresetOrigin::System};
        std::string root_id;
        std::string id;
        std::string name;
        std::optional<Domain::Preset::ConditionMatchMode> match_mode{
            Domain::Preset::ConditionMatchMode::FirstMatch
        };
        std::vector<const std::string*> conditions;
        Domain::Preset::PresetValueMap values;
        Domain::Preset::FeatureValueMap features;
        SourceLocation last_node_location;
        std::optional<std::string> user_file;

        bool has_same_values(const EvalPresetContext& rhs) const;
    };

    using EvalPresetContexts = std::vector<EvalPresetContext>;

    void build_named_presets();
    void collect_preset_ids(PresetKind kind, const PresetNode& node, const PresetNodePath& node_path);
    const PresetNode* find_node(PresetKind kind, std::string_view name) const;

    template <typename FdmConfigType, typename SlaConfigType>
    static Domain::Preset::EvaluatedPreset<FdmConfigType, SlaConfigType> preset_from_context(
        const Domain::Preset::HwPrinterConfig& hw_config,
        Domain::Preset::PresetKind kind,
        const EvalPresetContext& context
    );

    static EvalPresetContexts merged_same_presets(EvalPresetContexts presets);

private:
    const Domain::Preset::PresetCollection& m_presets;
    IdentifiedPresetsCollection m_preset_ids;
    Expr::Eval m_eval;
};


}