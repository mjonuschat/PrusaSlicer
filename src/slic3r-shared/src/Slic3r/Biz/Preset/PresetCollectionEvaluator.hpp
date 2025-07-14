#pragma once

#include <utility>

#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"


namespace Slic3r::Biz::Preset {

using ValueMaps = std::vector<Expr::ValueMap>;
enum class ExprCombine
{
    Or, And
};


class PresetCollectionEvaluator {
public:
    PresetCollectionEvaluator(
        const Domain::Preset::Presets& presets,
        const PresetEvaluator::NamedPresets& named_presets,
        Expr::Eval eval,
        const Expr::ValueMap& overrides
    );

    PresetEvaluator::EvalPresetContexts eval_preset(const ValueMaps& overrides, bool only_public = true, ExprCombine expr_combine = ExprCombine::Or) const;

private:
    PresetEvaluator::EvalPresetContexts eval_preset(
        const Domain::Preset::PresetNode& node,
        const PresetEvaluator::EvalPresetContexts& parent_contexts,
        const ValueMaps& overrides,
        ExprCombine expr_combine = ExprCombine::Or,
        bool skip_condition_eval = false
    ) const;
    bool eval_condition(const Expr::ValueMap& overrides, const Domain::Preset::SourceLocatedExpr& expr) const;
    bool eval_condition(const ValueMaps& overrides, ExprCombine expr_combine, const Domain::Preset::SourceLocatedExpr& expr) const;

    const PresetEvaluator::PresetNodePath& named_preset(const std::string& id) const;

private:
    const Domain::Preset::Presets& m_presets;
    const PresetEvaluator::NamedPresets& m_named_presets;
    Expr::Eval m_eval;
};

}

