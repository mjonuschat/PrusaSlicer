#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include <fmt/ranges.h>
#include "Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp"
#include "Slic3r/Biz/Preset/ValueMapBuilder.hpp"
#include "Slic3r/Uuid.hpp"
#include "Slic3r/TypeInfo.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::Biz::Preset {

namespace {
template <typename T, typename... Ts>
struct AnyTypeOf
{
    static constexpr bool value = (std::is_same<T, Ts>::value || ...);
};

template <typename FromType, typename ToType>
struct ValueCast
{
    static constexpr bool defined = false;
};

#define VALUE_CAST_DEF_STATIC(FromType, ToType)                                         \
template <>                                                                             \
struct ValueCast<FromType, ToType>                                                      \
{                                                                                       \
    static constexpr bool defined = true;                                               \
    static constexpr ToType cast(const FromType& v) { return static_cast<ToType>(v); }  \
};

VALUE_CAST_DEF_STATIC(double, bool);
VALUE_CAST_DEF_STATIC(double, int);
VALUE_CAST_DEF_STATIC(double, std::optional<int>);

template <>
struct ValueCast<double, Domain::Percentage>
{
    static constexpr bool defined = true;

    static constexpr Domain::Percentage cast(double v)
    {
        return Domain::Percentage{v};
    }
};

template <>
struct ValueCast<double, Domain::FloatOrPercentage>
{
    static constexpr bool defined = true;

    static Domain::FloatOrPercentage cast(double v)
    {
        return Domain::FloatOrPercentage{v};
    }
};

template <>
struct ValueCast<Domain::Percentage, Domain::FloatOrPercentage>
{
    static constexpr bool defined = true;

    static Domain::FloatOrPercentage cast(Domain::Percentage v)
    {
        return Domain::FloatOrPercentage{v};
    }
};

template <typename FromScalarT, typename ToScalarT>
    requires ValueCast<FromScalarT, ToScalarT>::defined
struct ValueCast<std::vector<FromScalarT>, std::vector<ToScalarT>>
{
    static constexpr bool defined = true;

    static constexpr std::vector<ToScalarT> cast(const std::vector<FromScalarT>& v)
    {
        std::vector<ToScalarT> dest;
        dest.reserve(v.size());
        for (const auto& vi : v)
            dest.push_back(ValueCast<FromScalarT, ToScalarT>::cast(vi));
        return dest;
    }
};

static_assert(ValueCast<std::vector<double>, std::vector<bool>>::defined);

#undef VALUE_CAST_DEF_STATIC

template <typename FromType>
struct CastingGetterVisitor
{
    Domain::ConfigItem& item;
    const FromType& value;

    template <typename ToType>
        requires ValueCast<FromType, ToType>::defined
    bool operator()(const ToType&)
    {
        item.set(ValueCast<FromType, ToType>::cast(value));
        return true;
    }

    template <typename ToType>
        requires(!ValueCast<FromType, ToType>::defined && std::is_same_v<FromType, ToType>)
    bool operator()(const ToType&)
    {
        item.set(value);
        return true;
    }

    template <typename ToType>
        requires(!ValueCast<FromType, ToType>::defined && !std::is_same_v<FromType, ToType>)
    bool operator()(const ToType& dest)
    {
        SPDLOG_ERROR(
            "Type mismatched for item {}: source type: {}  dest type: {}",
            item.name(),
            type_name(value),
            type_name(dest)
        );
        return false;
    }
};

struct ConfigValueSetterVisitor
{
    Domain::ConfigItem& item;
    const bool is_override;

    explicit ConfigValueSetterVisitor(const Domain::ContainsResult& result) :
        item(*result.item),
        is_override(result.is_override)
    {}

    /*
    std::monostate,
    Bools, Doubles, Ints, OptInts, Percentages, Vec2ds, Strings,
    bool, double, int, Percentage, Vec2d, std::string
     */
    bool operator()(const std::monostate& v)
    {
        if (!is_override) {
            item.set<std::optional<int>>(std::nullopt);
            return true;
        }
        return false;
    }

    template <typename ValueType>
        requires AnyTypeOf<ValueType, double, Domain::Percentage>::value
    bool operator()(const ValueType& v)
    {
        return item.visit(CastingGetterVisitor<ValueType>{item, v});
    }

    template <typename ValueType>
        requires AnyTypeOf<ValueType, std::string>::value
    bool operator()(const ValueType& v)
    {
        if (!item.holds_alternative<ValueType>() && !item.holds_alternative<Domain::EnumWrapper>()) {
            std::string dest_type_name = item.value().visit([](const auto& v) {
                return type_name(v);
            });
            SPDLOG_ERROR(
                "Type mismatched for item {}: source type: {}  dest type: {}",
                item.name(),
                type_name(v),
                dest_type_name
            );
            return false;
        }
        item.set(v);
        return true;
    }

    template <typename ValueType>
        requires AnyTypeOf<ValueType, Domain::Preset::Strings>::value
    bool operator()(const ValueType& v)
    {
        if (item.holds_alternative<std::string>()) {
            item.set(fmt::format("{}", fmt::join(v, ",")));
        } else if (!item.holds_alternative<ValueType>()
                   && !item.holds_alternative<Domain::EnumVectorWrapper>())
        {
            std::string dest_type_name = item.value().visit([](const auto& v) {
                return type_name(v);
            });
            SPDLOG_ERROR(
                "Type mismatched for item {}: source type: {}  dest type: {}",
                item.name(),
                type_name(v),
                dest_type_name
            );
            return false;
        } else
            item.set(v);
        return true;
    }

    template <typename ValueType>
        requires AnyTypeOf<
            ValueType,
            Domain::Vec2ds
        >::value
    bool operator()(const ValueType& v)
    {
        if (item.holds_alternative<std::string>()) {
            std::vector<std::string> values;
            for (const auto& vi : v)
                values.emplace_back(fmt::format("{}x{}", vi[0], vi[1]));
            item.set(fmt::format("{}", fmt::join(values, ",")));
        } else if (!item.holds_alternative<ValueType>()) {
            std::string dest_type_name = item.value().visit([](const auto& v) {
                return type_name(v);
            });
            SPDLOG_ERROR(
                "Type mismatched for item {}: source type: {}  dest type: {}",
                item.name(),
                type_name(v),
                dest_type_name
            );
            return false;
        } else
            item.set(v);
        return true;
    }

    template <typename ValueType>
        requires AnyTypeOf<
            ValueType,
            Domain::Preset::Bools,
            Domain::Preset::Doubles,
            Domain::Preset::Ints,
            Domain::Preset::OptInts,
            Domain::Preset::Percentages,
            bool,
            int,
            Domain::Vec2d>::value
    bool operator()(const ValueType& v)
    {
        if (item.holds_alternative<ValueType>()) {
            item.set(v);
            return true;
        }


        if constexpr (Domain::is_std_vector_v<ValueType>) {
            const bool set = item.visit(
                [&v, &item=this->item]<typename T>(const T&) -> bool
                {
                    if constexpr (Domain::is_std_vector_v<T> && ValueCast<ValueType, T>::defined) {
                        item.set(ValueCast<ValueType, T>::cast(v));
                        return true;
                    }
                    return false;
                }
            );
            if (set)
                return true;
        }

        std::string dest_type_name = item.value().visit([](const auto& v) {
            return type_name(v);
        });
        SPDLOG_ERROR(
            "Type mismatched for item {}: source type: {}  dest type: {}",
            item.name(),
            type_name(v),
            dest_type_name
        );
        return false;

    }
};

template <typename ConfigType>
    requires std::is_base_of_v<Domain::ConfigBox, ConfigType>
ConfigType config_values(const Domain::Preset::PresetValueMap& values)
{
    ConfigType config;
    for (const auto& [k, v] : values) {
        const auto q = config.contains(k);
        if (q.item == nullptr) {
            SPDLOG_ERROR("Invalid key {} for {}", k, type_name(config));
            continue;
        }

        // if value was written and this is override, we need to enable that override
        if (std::visit(ConfigValueSetterVisitor{q}, v) && q.is_override) {
            config.overrides.enable(q.item->name());
        }
    }
    return config;
}

template <typename FdmConfigType, typename SlaConfigType>
Domain::Preset::EvaluatedPreset<FdmConfigType, SlaConfigType>::PresetValues config_values(
    Domain::PrinterTechnology technology,
    const Domain::Preset::PresetValueMap& values
)
{
    if (technology == Domain::PrinterTechnology::FFF) {
        if constexpr (std::is_same_v<FdmConfigType, std::monostate>)
            PANIC("Unsupported config type");
        else
            return config_values<FdmConfigType>(values);
    }
    if (technology == Domain::PrinterTechnology::SLA) {
        if constexpr (std::is_same_v<SlaConfigType, std::monostate>)
            PANIC("Unsupported config type");
        else
            return config_values<SlaConfigType>(values);
    }
    PANIC("Unsupported printer technology");
}

} // namespace

bool PresetEvaluator::EvalPresetContext::has_same_values(const EvalPresetContext& rhs) const
{
    // last_node_location and conditions left intentionally
    return root_id == rhs.root_id
        && id == rhs.id
        && name == rhs.name
        && match_mode == rhs.match_mode
        && values == rhs.values
        && features == rhs.features;
}

template <typename FdmConfigType, typename SlaConfigType>
Domain::Preset::EvaluatedPreset<FdmConfigType, SlaConfigType> PresetEvaluator::preset_from_context(
    Domain::PrinterTechnology technology,
    Domain::Preset::PresetKind kind,
    const EvalPresetContext& context
)
{
    return {
        .kind       = kind,
        .root_id   = context.root_id,
        .id         = context.id.empty() ? generate_uuid() : context.id,
        .name       = context.name,
        .values     = config_values<FdmConfigType, SlaConfigType>(technology, context.values),
        .features   = context.features,
        .conditions = context.conditions,
        .last_node_location = context.last_node_location
    };
}

PresetEvaluator::EvalPresetContexts PresetEvaluator::merged_same_presets(const EvalPresetContexts& presets)
{
    EvalPresetContexts ret;

    for (const auto& p : presets) {
        const bool is_unique =
            std::ranges::none_of(ret, [&p](const auto& other) { return p.has_same_values(other); });
        if (is_unique) {
            ret.emplace_back(p);
        }
    }
    std::ranges::sort(
        ret,
        [](const auto& a, const auto& b)
        { return a.name < b.name || (a.name == b.name && a.id < b.id); }
    );

    return ret;
}

void PresetEvaluator::build_named_presets()
{
    m_named_presets.clear();
    for (const auto& [kind, presets] : m_presets)
        for (const auto& p : presets)
            collect_named_presets(kind, p, {&p});
}

void PresetEvaluator::collect_named_presets(
    PresetKind kind,
    const PresetNode& node,
    const PresetNodePath& node_path
)
{
    if (!node.id.empty()) {
        m_named_presets[kind].emplace(std::make_pair(node.id, node_path));
    }

    for (const auto& v : node.variants) {
        PresetNodePath child_path = node_path;
        child_path.push_back(&v);
        collect_named_presets(kind, v, child_path);
    }
}

const Domain::Preset::PresetNode* PresetEvaluator::find_node(PresetKind kind, std::string_view name) const
{
    auto presets_it = m_presets.find(kind);
    if (presets_it == m_presets.end())
        return nullptr;
    const auto& presets = presets_it->second;
    auto it             = std::find_if(presets.begin(), presets.end(), [&name](const auto& preset) {
        return preset.name == name;
    });
    if (it == presets.end())
        return nullptr;
    return &*it;
}

PresetEvaluator::EvaluatedPrinterPresets PresetEvaluator::evaluate(const HwPrinterConfig& hw_config) const
{
    Expr::ValueMap printer_values;
    append_printer_values(printer_values, hw_config);

    ValueMaps printer_tools_values;
    for (const auto& tool : hw_config.tools) {
        Expr::ValueMap tool_values = printer_values;

        append_tool_values(tool_values, tool);
        printer_tools_values.emplace_back(tool_values);
    }

    // 1. Printer preset
    PresetKind printer_kind = Domain::Preset::printer_kind(hw_config.technology);
    auto printers_it        = m_presets.find(printer_kind);
    auto printer_names_it   = m_named_presets.find(printer_kind);
    ASSERT(printers_it != m_presets.end() && printer_names_it != m_named_presets.end());

    PresetCollectionEvaluator printer_eval(printers_it->second, printer_names_it->second, m_eval, {});
    auto printer_presets = printer_eval.eval_preset({printer_tools_values});
    if (printer_presets.empty())
        SPDLOG_ERROR(
            "No printer presets available for configuration {} ({}) referring to printer {}",
            hw_config.name,
            hw_config.id,
            hw_config.printer_id
        );

    EvaluatedPrinterPresets ret;

    for (const auto& printer_preset : printer_presets) {
        EvaluatedPrinterPreset ep{.hw_config = hw_config};
        ep.preset = preset_from_context<Domain::PrinterSettings, Domain::SLAPrinterSettings>(
            hw_config.technology,
            printer_kind,
            printer_preset
        );

        // 2. Print preset
        PresetKind print_kind = Domain::Preset::print_kind(hw_config.technology);
        auto prints_it        = m_presets.find(print_kind);
        auto print_names_it   = m_named_presets.find(print_kind);
        ASSERT(prints_it != m_presets.end() && print_names_it != m_named_presets.end());

        PresetCollectionEvaluator print_eval(prints_it->second, print_names_it->second, m_eval, {});
        auto print_presets = print_eval.eval_preset(printer_tools_values);

        // 3. Tool print presets
        // for each tool
        PresetKind tool_kind = Domain::Preset::tool_print_kind(hw_config.technology);
        auto tool_it         = m_presets.find(tool_kind);
        auto tool_names_it   = m_named_presets.find(tool_kind);
        ASSERT(hw_config.technology == Domain::PrinterTechnology::SLA || (tool_it != m_presets.end() && tool_names_it != m_named_presets.end()));

        for (const auto& print_preset : print_presets) {
            auto evaluated_print_preset = preset_from_context<Domain::PrintSettings, Domain::SLAPrintSettings>(
                hw_config.technology,
                print_kind,
                print_preset
            );
            Domain::Preset::AllToolsEvaluatedToolPrintPresets tools;

            Expr::ValueMap print_values = printer_values;
            std::visit([&print_values](const auto& v) {
                append_print_values(print_values, v);
            }, evaluated_print_preset.values);

            if (hw_config.technology == Domain::PrinterTechnology::FFF) {
                PresetCollectionEvaluator tool_eval(tool_it->second, tool_names_it->second, m_eval, {});
                for (const auto& tool : hw_config.tools) {
                    Expr::ValueMap tool_values = print_values;
                    append_tool_values(tool_values, tool);

                    // evaluate all variants
                    Domain::Preset::SingleToolEvaluatedToolPrintPresets eval_variants;
                    auto tool_preset_variants = tool_eval.eval_preset({tool_values});
                    for (const auto& tool_variant : tool_preset_variants)
                        eval_variants.emplace_back(
                            preset_from_context<Domain::ToolPrintSettings, std::monostate>(
                                hw_config.technology,
                                tool_kind,
                                tool_variant
                            )
                        );
                    tools.emplace_back(std::move(eval_variants));
                }
            }

            // 4. Material
            PresetKind mat_kind = Domain::Preset::material_kind(hw_config.technology);
            auto mats_it        = m_presets.find(mat_kind);
            auto mat_names_it   = m_named_presets.find(mat_kind);
            ASSERT(mats_it != m_presets.end() && mat_names_it != m_named_presets.end());

            Domain::Preset::AllToolsEvaluatedMaterialPresets materials;
            for (const auto& tool : hw_config.tools) {
                Expr::ValueMap tool_values = print_values;
                append_tool_values(tool_values, tool);
                Domain::Preset::SingleToolEvaluatedMaterialPresets variants;

                PresetCollectionEvaluator material_eval(mats_it->second, mat_names_it->second, m_eval, {});
                auto mat_presets = material_eval.eval_preset({tool_values});
                for (const auto& mat : mat_presets) {
                    variants.emplace_back(
                        preset_from_context<Domain::FilamentSettings, Domain::SLAMaterialSettings>(
                            hw_config.technology,
                            mat_kind,
                            mat
                        )
                    );
                }
                materials.emplace_back(std::move(variants));
            }

            // For FFF add only prints with tools filled
            if (hw_config.technology != Domain::PrinterTechnology::FFF
                || std::ranges::all_of(tools, [](const auto& t) { return !t.empty(); }))
            {
                ep.prints.emplace_back(
                    std::move(evaluated_print_preset),
                    std::move(tools),
                    std::move(materials)
                );
            } else if (hw_config.technology != Domain::PrinterTechnology::FFF) {
                SPDLOG_WARN(
                    "Print preset {} for printer {} was removed as it has at least one tool without tool print presets",
                    evaluated_print_preset.name,
                    printer_preset.name
                );
            }
        }
        ret.emplace_back(std::move(ep));
    }
    return ret;
}

} // namespace Slic3r::Biz::Preset
