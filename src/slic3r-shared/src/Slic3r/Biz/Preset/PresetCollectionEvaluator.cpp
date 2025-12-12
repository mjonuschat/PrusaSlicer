#include "Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp"

#include "libslic3r/CustomParametersHandling.hpp"

// Xcode 15 has not finished ranges support we need here
#include <version>
#if !defined(__cpp_lib_ranges) \
    || __cpp_lib_ranges < 201'911L \
    || (__clang_major__ < 16 && defined(__apple_build_version__))
#define HAS_RANGES_VIEWS 0
#else
#define HAS_RANGES_VIEWS 1
#include <ranges>
#endif

#include <fmt/format.h>

#include <Slic3r/Log.hpp>

#define DEBUG_CONDITION_EVAL 0

namespace Slic3r::Biz::Preset {

namespace {
    using namespace Domain::Preset;

    void override_preset_values(PresetValueMap& dest, const PresetValueMap& overrides)
    {
        for (const auto& [key, value] : overrides) {
            if (key.starts_with("custom_parameters_")) {
                auto it = dest.find(key);
                ASSERT(std::holds_alternative<std::string>(value));
                if (it != dest.end()) {
                    ASSERT(std::holds_alternative<std::string>(it->second));
                    dest[key] = merge_json(std::get<std::string>(it->second), std::get<std::string>(value));
                    continue;
                }
            }
            dest[key] = value;
        }
    }

    void override_feature_values(FeatureValueMap& dest, const FeatureValueMap& overrides)
    {
        for (const auto& [key, value] : overrides)
            dest[key] = value;
    }
}

PresetCollectionEvaluator::PresetCollectionEvaluator(
    const Domain::Preset::Presets& presets,
    const PresetEvaluator::IdentifiedPresets& named_presets,
    Expr::Eval eval,
    const Expr::ValueMap& overrides
) :
    m_presets(presets),
    m_named_presets(named_presets),
    m_eval(std::move(eval))
{
    m_eval.set_vars(overrides);
#if DEBUG_CONDITION_EVAL
    m_eval.set_debug_output_enabled(true);
#endif
}

PresetEvaluator::EvalPresetContexts PresetCollectionEvaluator::eval_preset(
    const ValueMaps& overrides,
    bool only_public,
    ExprCombine expr_combine
) const
{
#if !HAS_RANGES_VIEWS
    PresetEvaluator::EvalPresetContexts ret;
    for (const auto& preset : m_presets) {
        auto eval_presets = eval_preset(preset, preset.id, preset.origin, {{preset.id}}, overrides);
        auto it           = only_public ?
                      std::remove_if(
                eval_presets.begin(),
                eval_presets.end(),
                [](const auto& ep) { return !Domain::Preset::is_public_name(ep.name); }
            ) :
                      eval_presets.end();

        ret.insert(ret.end(), std::make_move_iterator(eval_presets.begin()), std::make_move_iterator(it));
    }
    return ret;
#else
    auto joined_view =
        m_presets
        | std::views::transform(
            [&](const auto& preset)
            {
                return eval_preset(
                           preset,
                           preset.id,
                           preset.origin,
                           preset.user_file,
                           {{
                             .origin    = preset.origin,
                             .root_id   = preset.id,
                             .user_file = preset.user_file
                           }},
                           overrides.empty() ? ValueMaps{{}} : overrides,
                           expr_combine
                       )
                    | std::views::filter(
                           [only_public](const auto& ep)
                           { return !only_public || Domain::Preset::is_public_name(ep.name); }
                    );
            }
        )
        | std::views::join;

    PresetEvaluator::EvalPresetContexts ret;
    for (auto&& ep : joined_view)
        ret.push_back(std::move(ep));
    std::ranges::sort(
        ret,
        [](const auto& x, const auto& y)
        { return x.name < y.name || (x.name == y.name && x.id < y.id); }
    );

    return ret;
#endif
}

PresetEvaluator::EvalPresetContexts PresetCollectionEvaluator::eval_preset(
    const Domain::Preset::PresetNode& node,
    const std::string& root_id,
    PresetOrigin origin,
    std::optional<std::string> user_file,
    const PresetEvaluator::EvalPresetContexts& parent_contexts,
    const ValueMaps& overrides,
    ExprCombine expr_combine,
    bool skip_condition_eval
) const
{
    ASSERT(!parent_contexts.empty());

    if (!skip_condition_eval
        && node.condition.has_value()
        && !eval_condition(overrides, expr_combine, node.condition.value()))
        return {};

    PresetEvaluator::EvalPresetContexts ret = parent_contexts;

    if (!node.inherits.empty()) {
        for (const auto& inh : node.inherits) {
            const auto& node_path = named_preset(inh);
            ASSERT(node_path.size() == 1);

            // if this node has condition, it overrides the superclass condition
            // (and hence we want to skip its evaluation)
            const bool skip_superclass_condition_eval =
                skip_condition_eval || node.condition.has_value();
            ret = eval_preset(
                *node_path.front(),
                root_id,
                origin,
                user_file,
                ret,
                overrides,
                expr_combine,
                skip_superclass_condition_eval
            );

            if (ret.empty()) {
#if DEBUG_CONDITION_EVAL
                SPDLOG_DEBUG(
                    "Inherited node {} root condition fails => quitting evaluation of {}",
                    inh,
                    node.source_location.to_string()
                );
#endif
                return {};
            };
        }
    }

    Domain::Preset::PresetValueMap unconditional_inherited_values;
    Domain::Preset::FeatureValueMap unconditional_inherited_features;
    for (const auto& unc_inh : node.unconditional_inherits) {
        const auto& node_path = named_preset(unc_inh);

        for (const auto& n : node_path) {
            override_preset_values(unconditional_inherited_values, n->values);
            override_feature_values(unconditional_inherited_features, n->features);
        }
    }

    for (auto& context : ret) {
        if (!node.id.empty())
            context.id = Domain::Preset::derive_name(node.id, context.id);
        if (node.name.has_value())
            context.name = Domain::Preset::derive_name(node.name.value(), context.name);
        if (node.match_mode.has_value())
            context.match_mode = node.match_mode.value();
        if (node.name.has_value() && node.id.empty()) {
            context.id = Domain::Preset::derive_name(node.name.value(), context.id);
            SPDLOG_WARN(
                "{}: Preset node has defined name ({}) but not id! "
                "The id is derived from the name ({}) and MAY NOT BE UNIQUE.",
                node.source_location.to_string(),
                node.name.value(),
                context.id
             );
        }
        if (node.condition.has_value())
            context.conditions.push_back(*node.condition.value());
        context.last_node_location = node.source_location;
        override_preset_values(context.values, unconditional_inherited_values);
        override_preset_values(context.values, node.values);
        override_feature_values(context.features, unconditional_inherited_features);
        override_feature_values(context.features, node.features);
    }

    size_t conditional_variants   = 0;
    size_t unconditional_variants = 0;

    PresetEvaluator::EvalPresetContexts var_contexts;

    const bool first_match_only = std::ranges::all_of(
        ret,
        [](const auto& ctx)
        { return *ctx.match_mode == Domain::Preset::ConditionMatchMode::FirstMatch; }
    );

    // make sure that the match_mode is same for contexts
    ASSERT(
        first_match_only
        || std::ranges::none_of(
            ret,
            [](const auto& ctx)
            { return *ctx.match_mode == Domain::Preset::ConditionMatchMode::FirstMatch; }
        )
    );

    for (const auto& var : node.variants) {
        const bool conditional = var.condition.has_value();
        (conditional ? conditional_variants : unconditional_variants)++;
        if (conditional) {
            // There no unconditional variants allowed prior condition
            ASSERT(unconditional_variants == 0);

            // Condition is not met, continue with next variant
            if (!eval_condition(overrides, expr_combine, var.condition.value()))
                continue;
        } else if (conditional_variants > 0) {
            // if there was at least one condition case met before
            // only one unconditional variant is valid
            ASSERT(unconditional_variants == 1);
        }

        auto var_ctx = eval_preset(
            var,
            root_id,
            origin,
            user_file,
            {
                {.origin     = origin,
                 .root_id    = root_id,
                 .match_mode = first_match_only ? ConditionMatchMode::FirstMatch :
                                                  ConditionMatchMode::AllMatches,
                 .user_file  = user_file},

            },
            overrides,
            expr_combine,
            true
        );
        var_contexts.insert(
            var_contexts.end(),
            std::make_move_iterator(var_ctx.begin()),
            std::make_move_iterator(var_ctx.end())
        );

        // first successful condition met, break (switch-like statement behavior)
        if (conditional && first_match_only)
            break;
    }

    if (var_contexts.empty())
        return PresetEvaluator::merged_same_presets(ret);

    // resolve variants
    PresetEvaluator::EvalPresetContexts product;
    for (const auto& ctx : ret) {
        for (const auto& var_ctx : var_contexts) {
            PresetEvaluator::EvalPresetContext context = ctx;
            if (!var_ctx.id.empty())
                context.id   = Domain::Preset::derive_name(var_ctx.id, context.id);
            if (!var_ctx.name.empty())
                context.name = Domain::Preset::derive_name(var_ctx.name, context.name);
            if (var_ctx.match_mode.has_value())
                context.match_mode = var_ctx.match_mode;
            context.conditions.insert(
                context.conditions.end(),
                var_ctx.conditions.begin(),
                var_ctx.conditions.end()
            );
            context.last_node_location = var_ctx.last_node_location;
            override_preset_values(context.values, var_ctx.values);
            override_feature_values(context.features, var_ctx.features);

            product.emplace_back(std::move(context));
        }
    }

    return PresetEvaluator::merged_same_presets(product);
}

struct BoolCaster
{
    bool operator()(const std::string&) const
    {
        return false;
    }

    bool operator()(const Domain::Expr::RegEx&) const
    {
        return false;
    }

    bool operator()(const auto& v) const
    {
        return static_cast<bool>(v);
    }
};

bool PresetCollectionEvaluator::eval_condition(
    const Expr::ValueMap& overrides,
    const Domain::Preset::SourceLocatedExpr& expr
) const
{
    try {
        auto result = m_eval.eval(*expr, overrides);
        return boost::apply_visitor(BoolCaster{}, result);
    } catch (Expr::EvalError& e) {
        throw Expr::EvalError(fmt::format("[{}] {}", expr.source_location.to_string(), e.what()));
    }
}

bool PresetCollectionEvaluator::eval_condition(
    const ValueMaps& overrides,
    ExprCombine expr_combine,
    const Domain::Preset::SourceLocatedExpr& expr
) const
{
#if DEBUG_CONDITION_EVAL
    SPDLOG_DEBUG(
        "Evaluating expression defined in {}",
        expr.source_location.to_string(),
        Biz::Expr::to_string(expr.value)
    );
#endif

    for (const auto& var : overrides) {
        const bool val = eval_condition(var, expr);

#if DEBUG_CONDITION_EVAL
        SPDLOG_DEBUG("expression result: {}", val);
#endif

        if (val && expr_combine == ExprCombine::Or) {
#if DEBUG_CONDITION_EVAL
            SPDLOG_DEBUG("Final result: True (or combination)");
#endif

            return true;
        }
        if (!val && expr_combine == ExprCombine::And) {
#if DEBUG_CONDITION_EVAL
            SPDLOG_DEBUG("Final result: False (and combination)");
#endif

            return false;
        }
    }

#if DEBUG_CONDITION_EVAL
    SPDLOG_DEBUG(
        "Final result: {} ({} combination)",
        expr_combine == ExprCombine::And,
        expr_combine == ExprCombine::Or ? "or" : "and"
    );
#endif

    return expr_combine == ExprCombine::And;
}

const PresetEvaluator::PresetNodePath& PresetCollectionEvaluator::named_preset(const std::string& id) const
{
    auto it = m_named_presets.find(id);
    ASSERT(it != m_named_presets.end());
    return it->second;
}

} // namespace Slic3r::Biz::Preset
