#include "Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp"

#include <fmt/format.h>

namespace Slic3r::Biz::Preset {

PresetEvaluator::EvalPresetContexts PresetCollectionEvaluator::eval_preset(const Expr::ValueMap& overrides) const
{
    PresetEvaluator::EvalPresetContexts ret;
    for (const auto& preset : m_presets) {
        auto eval_presets = eval_preset(preset, {{}}, overrides);
        ret.insert(ret.end(), std::make_move_iterator(eval_presets.begin()), std::make_move_iterator(eval_presets.end()));
    }
    return ret;
}

PresetEvaluator::EvalPresetContexts PresetCollectionEvaluator::eval_preset(
    const Domain::Preset::PresetNode& node,
    const PresetEvaluator::EvalPresetContexts& parent_contexts,
    const Expr::ValueMap& overrides,
    bool skip_condition_eval
) const
{
    ASSERT(!parent_contexts.empty());

    if (!skip_condition_eval && node.condition.has_value() && !eval_condition(overrides, node.condition.value()))
        return {};

    PresetEvaluator::EvalPresetContexts ret = parent_contexts;

    if (!node.inherits.empty()) {
        for (const auto& inh : node.inherits) {
            const auto& node_path = named_preset(inh);
            ASSERT(node_path.size() == 1);
            ret = eval_preset(*node_path.front(), ret, overrides, skip_condition_eval);
        }
    }

    Domain::Preset::PresetValueMap unconditional_inherited_values;
    for (const auto& unc_inh : node.unconditional_inherits) {
        const auto& node_path = named_preset(unc_inh);

        for (const auto& n : node_path)
            Domain::Preset::override_values(unconditional_inherited_values, n->values);

    }

    for (auto& context : ret) {
        if (!node.id.empty())
            context.id = Domain::Preset::derive_name(node.id, context.id);
        if (node.name.has_value())
            context.name = Domain::Preset::derive_name(node.name.value(), context.name);
        if (node.condition.has_value())
            context.conditions.push_back(*node.condition.value());
        Domain::Preset::override_values(context.values, unconditional_inherited_values);
        Domain::Preset::override_values(context.values, node.values);
    }


    size_t conditional_variants = 0;
    size_t unconditional_variants = 0;

    PresetEvaluator::EvalPresetContexts var_contexts;

    for (const auto& var : node.variants) {
        const bool conditional = var.condition.has_value();
        (conditional ? conditional_variants : unconditional_variants)++;
        if (conditional) {
            // There no unconditional variants allowed prior condition
            ASSERT(unconditional_variants == 0);

            // Condition is not met, continue with next variant
            if (!eval_condition(overrides, var.condition.value()))
                continue;
        } else if (conditional_variants > 0) {
            // if there was at least one condition case met before
            // only one unconditional variant is valid
            ASSERT(unconditional_variants == 1);
        }

        auto var_ctx = eval_preset(var, {{}}, overrides, true);
        var_contexts.insert(
            var_contexts.end(),
            std::make_move_iterator(var_ctx.begin()),
            std::make_move_iterator(var_ctx.end())
        );

        // first successful condition met, break (switch-like statement behavior)
        if (conditional)
            break;
    }

    if (var_contexts.empty())
        return ret;

    // resolve variants
    PresetEvaluator::EvalPresetContexts product;
    for (const auto& ctx : ret) {
        for (const auto& var_ctx : var_contexts) {
            PresetEvaluator::EvalPresetContext context = ctx;
            context.id = Domain::Preset::derive_name(var_ctx.id, context.id);
            context.name = Domain::Preset::derive_name(var_ctx.name, context.name);
            context.conditions.insert(context.conditions.end(), var_ctx.conditions.begin(), var_ctx.conditions.end());
            Domain::Preset::override_values(context.values, var_ctx.values);

            product.emplace_back(std::move(context));
        }
    }

    return product;
}

bool PresetCollectionEvaluator::eval_condition(const Expr::ValueMap& overrides, const Domain::Preset::SourceLocatedExpr& expr) const
{
    try {
        auto result = m_eval.eval(*expr, overrides);
        return result.type() == typeid(bool) && boost::get<bool>(result);
    } catch (Expr::EvalError& e) {
        throw Expr::EvalError(fmt::format("[{}] {}", expr.source_location.to_string(), e.what()));
    }
}

const PresetEvaluator::PresetNodePath& PresetCollectionEvaluator::named_preset(const std::string& id) const
{
    auto it = m_named_presets.find(id);
    ASSERT(it != m_named_presets.end());
    return it->second;
}

}
