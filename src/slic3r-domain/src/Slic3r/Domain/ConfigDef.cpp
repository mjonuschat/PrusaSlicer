#include "Slic3r/Domain/ConfigDef.hpp"

namespace Slic3r::Domain {
ConfigDefinitions::ConfigDefinitions(
    const std::vector<std::string>& acceptable_boxes, std::function<void(ConfigDefinitions&)> init_fn
)
{
    m_acceptable_boxes = acceptable_boxes;
    init_fn(*this);
    std::sort(m_defs.begin(), m_defs.end());
    this->check_valid();
    m_finalized = true;
}

ConfigItemDef* ConfigDefinitions::add(const std::string_view name, const std::type_info& type)
{
    ASSERT(!m_finalized);
    return &m_defs.emplace_back(ConfigItemDef{std::string(name), &type});
}

void ConfigDefinitions::check_valid() const
{
    ASSERT(std::is_sorted(m_defs.begin(), m_defs.end()));
    ASSERT(
        std::adjacent_find(
            m_defs.begin(), m_defs.end(), // check for duplicates
            [](const auto& a, const auto& b) { return a.name == b.name; }
        ) == m_defs.end()
    );

    for (const ConfigItemDef& def : m_defs) {
        ASSERT(def.type != nullptr);
        ASSERT(!def.location.empty());
        ASSERT(std::ranges::find(def.overrides_in, def.location) == def.overrides_in.end());

        // Check that all items are assigned to valid boxes.
        ASSERT(std::any_of(m_acceptable_boxes.begin(), m_acceptable_boxes.end(), [&def](const auto& b) {
            return def.location == b;
        }));
        ASSERT(std::all_of(def.overrides_in.begin(), def.overrides_in.end(), [this](const auto& box) {
            return std::any_of(
                m_acceptable_boxes.begin(), m_acceptable_boxes.end(),
                [&box](const auto& b) { return box == b; }
            );
        }));

        if (def.init_fn) {
            const ConfigValue value{def.init_fn()};
            value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });
        } else if (def.init_fn_ex) {
            const ConfigValue value{def.init_fn_ex(def.location)};
            value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });

            for (const std::string& override_location : def.overrides_in) {
                const ConfigValue value{def.init_fn_ex(override_location)};
                value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });
            }
        } else {
            PANIC("init_fn or init_fn_ex must be defined");
        }

        // Check that all choices (if used) have the same key type and that it matches the item type.
        if (!def.choices.empty()) {
            for (const auto& [value, str] : def.choices) {
                ASSERT(
                    (*def.type == typeid(std::string) && std::holds_alternative<std::string>(value)
                    ) ||
                    (*def.type == typeid(int) && std::holds_alternative<int>(value)) ||
                    (*def.type == typeid(double) && std::holds_alternative<double>(value)) ||
                    (*def.type == typeid(Percentage) && std::holds_alternative<double>(value)) ||
                    (*def.type == typeid(FloatOrPercentage) && std::holds_alternative<double>(value))
                );
            }
        }
    }
}
} // namespace Slic3r::Domain
