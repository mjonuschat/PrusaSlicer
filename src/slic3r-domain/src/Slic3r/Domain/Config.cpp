#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <algorithm>

namespace std {

    template<> struct hash<Slic3r::Domain::Percentage> {
        std::size_t operator()(const Slic3r::Domain::Percentage& v) const noexcept {
            return std::hash<double>{}(v.value);
        }
    };

    template<> struct hash<Slic3r::Domain::FloatOrPercentage> {
        std::size_t operator()(const Slic3r::Domain::FloatOrPercentage& v) const noexcept {
            const std::size_t seed{std::hash<double>{}(v.get_abs_value(1.0))};
            return v.is_percentage() ? seed ^ 0x9e3779b9 : seed;
        }
    };

    template<> struct hash<Slic3r::Domain::Vec2d> {
        std::size_t operator()(const Slic3r::Domain::Vec2d& v) const noexcept {
            std::size_t seed{std::hash<double>{}(v.x())};
            boost::hash_combine(seed, std::hash<double>{}(v.y()));
            return seed;
        }
    };

    template<> struct hash<Slic3r::Domain::Vec3d> {
        std::size_t operator()(const Slic3r::Domain::Vec3d& v) const noexcept {
            std::size_t seed{std::hash<double>{}(v.x())};
            boost::hash_combine(seed, std::hash<double>{}(v.y()));
            boost::hash_combine(seed, std::hash<double>{}(v.z()));
            return seed;
        }
    };
}

namespace Slic3r::Domain {

ConfigItem::ConfigItem(const ConfigItemDef& def, ConfigLocation location)
    : m_value{def.init_fn ? def.init_fn() : def.init_fn_ex(location)}, m_current_location{location}, m_def{&def}
{}

CompatibilityRule ConfigItem::compatibility_rule() const
{
    const CompatibilityRules& rules       = get_compatibility_rules();
    CompatibilityRules::const_iterator it = rules.find(name());
    return it == rules.cend() ? CompatibilityRule::Undefined : it->second;
}

ConfigItems::ConfigItems(const ConfigDefinitions& defs, const ConfigLocation& location) :
    m_location{location}
{
    for (const ConfigItemDef& def : defs.defs()) {
        if (def.location == location) {
            m_items.push_back(ConfigItem(def, location));
        }
    }
}

const ConfigItem& ConfigItems::opt(const std::string_view key) const { return const_cast<ConfigItems*>(this)->opt(key); }

ConfigItem& ConfigItems::opt(const std::string_view key) {
    auto it = std::lower_bound(m_items.begin(), m_items.end(), key,
        [](const ConfigItem& i, const auto& val) { return i.def().name < val; });
    if (it != m_items.end() && it->def().name == key)
        return *it;
    PANIC("Option not found", key);
    throw std::exception(); // to silence a warning
}

namespace {
template<typename Range>
std::optional<std::size_t> find_item_index(Range& items, const std::string& key) {
    const auto it{std::lower_bound(
        items.begin(), items.end(), key,
        [](const ConfigItem& i, const auto& val) { return i.def().name < val; }
    )};
    if (it == items.end() || it->name() != key) {
        return std::nullopt;
    }
    return std::distance(items.begin(), it);
}

template<typename Range>
auto find_item(Range& items, const std::string& key) -> decltype(&(*items.begin())) {
    if (const auto index{find_item_index(items, key)}) {
        return &items[*index];
    }
    return nullptr;
}

template <typename K, typename V>
std::vector<K> diff_keys(
    const std::map<K, V>& a,
    const std::map<K, V>& b,
    std::function<bool(const V&, const V&)> equals_comp
)
{
    std::vector<K> result;

    auto a_it{a.begin()};
    auto b_it{b.begin()};
    const auto a_end{a.end()};
    const auto b_end{b.end()};

    while(a_it != a_end && b_it != b_end) {
        const auto& [a_key, a_value]{*a_it};
        const auto& [b_key, b_value]{*b_it};

        if (a_key < b_key) {
            result.push_back(a_key);
            ++a_it;
        } else if (a_key > b_key) {
            result.push_back(b_key);
            ++b_it;
        } else {
            if (!(equals_comp(a_value, b_value))) {
                result.push_back(a_key);
            }
            ++a_it;
            ++b_it;
        }
    }

    while (a_it != a.end()) {
        result.push_back(a_it->first);
        ++a_it;
    }
    while (b_it != b.end()) {
        result.push_back(b_it->first);
        ++b_it;
    }

    return result;
}
}

ConfigItem* ConfigItems::find(const std::string& key) {
    return find_item(m_items, key);
}

const ConfigItem* ConfigItems::find(const std::string& key) const {
    return find_item(m_items, key);
}

const std::vector<ConfigItem>& ConfigItems::all_items() const
{
    return m_items;
}

std::vector<ConfigItem>& ConfigItems::all_items()
{
    return m_items;
}

std::vector<std::string> ConfigItems::diff_keys(const ConfigItems& other) const {
    ASSERT(m_location == other.m_location && m_items.size() == other.m_items.size());

    std::vector<std::string> result;

    for (std::size_t i{}; i < m_items.size(); ++i) {
        const ConfigItem& a{m_items[i]};
        const ConfigItem& b{other.m_items[i]};
        ASSERT(a.name() == b.name());
        if (a != b) {
            result.push_back(a.name());
        }
    }
    return result;
}

ConfigOverrides::ConfigOverrides(const ConfigDefinitions& defs, const ConfigLocation location) {
    for (const ConfigItemDef& def : defs.defs()) {
        if (def.overrides_in.contains(location)) {
            m_items.emplace_back(ConfigItem(def, location));
        }
    }
}

void ConfigOverrides::disable(const std::string& key) {
    m_used_overrides.erase(key);
}

void ConfigOverrides::enable(const std::string& key) {
    const auto item_index{find_item_by_index(key)};
    m_used_overrides.insert({key, item_index});
}

std::size_t ConfigOverrides::size() const {
    return m_used_overrides.size();
}

const bool ConfigOverrides::empty() const {
    return m_used_overrides.empty();
}

std::optional<ConfigItem> ConfigOverrides::get(const std::string& key) const {
    const auto it{m_used_overrides.find(key)};
    if (it == m_used_overrides.end()) {
        return std::nullopt;
    }
    return m_items.at(it->second);
}

ConfigItem* ConfigOverrides::find(const std::string& key) {
    return find_item(m_items, key);
}
const ConfigItem* ConfigOverrides::find(const std::string& key) const {
    return find_item(m_items, key);
}

std::vector<std::reference_wrapper<const ConfigItem>> ConfigOverrides::overriden_items() const {
    std::vector<std::reference_wrapper<const ConfigItem>> result;

    std::ranges::transform(m_used_overrides, std::back_inserter(result), [&](const auto& pair) {
        return std::ref(m_items.at(pair.second));
    });

    return result;
}

std::vector<ConfigItem>& ConfigOverrides::all_items() {
    return m_items;
}

const std::vector<ConfigItem>& ConfigOverrides::all_items() const {
    return m_items;
}

std::size_t ConfigOverrides::find_item_by_index(const std::string& key) const {
    const auto index{find_item_index(m_items, key)};
    ASSERT(index, "The key does not belong to this!");
    return *index;
}

std::vector<std::string> ConfigOverrides::diff_overriden_keys(const ConfigOverrides& other) const
{
    return diff_keys<std::string, std::size_t>(
        m_used_overrides,
        other.m_used_overrides,
        [&](std::size_t index_a, std::size_t index_b)
        { return m_items.at(index_a) == other.m_items.at(index_b); }
    );
}

FindResult ConfigBox::find(const std::string &key)
{
    if (auto* item{overrides.find(key)}) {
        return {item, true};
    }
    return {items.find(key), false};
}

ConstFindResult ConfigBox::find(const std::string &key) const
{
    if (auto* item{overrides.find(key)}) {
        return {item, true};
    }
    return {items.find(key), false};
}

std::vector<std::string> ConfigBox::diff_keys(const ConfigBox& other) const
{
    std::vector<std::string> result{items.diff_keys(other.items)};
    const std::vector<std::string> overriden_diff_keys{
        overrides.diff_overriden_keys(other.overrides)
    };

    result.insert(result.end(), overriden_diff_keys.begin(), overriden_diff_keys.end());

    return result;
}

SquashedConfig::SquashedConfig(
    const BoxOrBoxesVector& all_boxes,
    const std::vector<unsigned>& extruder_candidates,
    const ConfigLocationSizes& location_sizes
)
{
    for (const auto& box_or_boxes : all_boxes) {
        std::visit([&](auto&& box_or_boxes) {
            using ValueType = std::remove_cvref_t<decltype(box_or_boxes)>;
            if constexpr (std::is_same_v<ValueType, BoxRef>) {
                this->add(box_or_boxes.get(), location_sizes);
            } else if constexpr (std::is_same_v<ValueType, BoxRefs>) {
                this->add(box_or_boxes, extruder_candidates, location_sizes);
            }
        }, box_or_boxes);
    }
}

std::vector<std::string> SquashedConfig::diff_keys(const SquashedConfig& other) const {
    return ::Slic3r::Domain::diff_keys<std::string, ConfigValue>(
        m_values,
        other.m_values,
        [](const ConfigValue& a, const ConfigValue& b) { return a == b; }
    );
}

bool SquashedConfig::operator==(const SquashedConfig& other) const {
    return diff_keys(other).empty();
}

const std::map<std::string, ConfigValue>& SquashedConfig::values() const {
    return m_values;
}

LocationSize get_max_location_size(
    const ConfigItemDef& def, const ConfigLocationSizes& location_sizes
)
{
    if (def.require_compatibility_rule) {
        return std::nullopt;
    }

    LocationSize result{location_sizes.at(def.location)};
    for (const ConfigLocation& location : def.overrides_in) {
        const auto location_size_it{location_sizes.find(location)};
        ASSERT(location_size_it != location_sizes.end(), "Location size must be provided!");
        const LocationSize& size{location_size_it->second};
        if (size) {
            if (result) {
                result = std::max(*result, *size);
            } else {
                result = size;
            }
        }
    }
    return result;
}

using ConfigItemRef = std::reference_wrapper<const ConfigItem>;
ConfigValue extract_values(const std::vector<ConfigItemRef>& items) {
    return items.front().get().visit([&](auto&& value) -> ConfigValue {
        using ValueType = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            std::vector<int> enum_values;
            std::ranges::transform(items, std::back_inserter(enum_values), [](const ConfigItemRef& item) {
                return item.get().get<EnumWrapper>().value();
            });

            const auto first_enum{items.front().get().get<EnumWrapper>()};
            const EnumVectorWrapper
                enum_vector{enum_values, first_enum.type(), first_enum.def()};
            return ConfigValue{enum_vector};
        } else if constexpr (
            std::is_same_v<ValueType, EnumVectorWrapper>
            || is_std_vector_v<ValueType>
        ) {
            // This can be implementd later simply by adding variants of ConfigValue.
            // The only special case is the EnumVectorWrapper.
            PANIC("Vectors of vectors are not supported!");
        } else {
            std::vector<ValueType> item_values;
            std::ranges::transform(items, std::back_inserter(item_values), [](const ConfigItemRef& item) {
                return item.get().get<ValueType>();
            });
            return ConfigValue{item_values};
        }
    });
}

ConfigValue extract_values(const std::vector<ConfigItem>& items) {
    std::vector<ConfigItemRef> refs;
    refs.insert(refs.end(), items.begin(), items.end());
    return extract_values(refs);
}

ConfigValue spread_values(const ConfigItem& item, const std::size_t size) {
    const std::vector<ConfigItem> items(size, item);
    return extract_values(items);
}

void SquashedConfig::add(const ConfigBox& box, const ConfigLocationSizes& location_sizes)
{
    for (const ConfigItem& item : box.items.all_items()) {
        const LocationSize location_size{get_max_location_size(item.def(), location_sizes)};
        m_values.insert({item.name(), location_size ? spread_values(item, *location_size) : item.value()});
    }
    for (const auto& item : box.overrides.overriden_items()) {
        const LocationSize location_size{get_max_location_size(item.get().def(), location_sizes)};
        m_values.insert_or_assign(
            item.get().name(),
            location_size ? spread_values(item.get(), *location_size) : item.get().value()
        );
    }
}

template<typename It, typename Func>
void iterate_together(
    const std::vector<It>& begins,
    const std::vector<It>& ends,
    Func&& func
) {
    std::vector<It> its{begins};
    const auto end_reached{[&](){
        for (std::size_t i{}; i < its.size(); ++i) {
            if (its[i] == ends[i]) {
                return true;
            }
        }
        return false;
    }};

    const auto increment_all{[&](){
        for (It& it : its) {
            ++it;
        }
    }};

    for (; !end_reached(); increment_all()) {
        std::vector<typename It::value_type> items;
        std::ranges::transform(its, std::back_inserter(items), [](const It& it) { return *it; });
        func(items);
    }
}

static Percentage get_average(const std::vector<Percentage>& values)
{
    double sum{};
    for (const Percentage& value : values) {
        sum += value.value;
    }
    return {sum / (double) values.size()};
}

static double get_average(const std::vector<double>& values)
{
    double sum{};
    for (const double& value : values) {
        sum += value;
    }
    return sum / (double) values.size();
}

template <typename T>
static T get_min(const std::vector<T>& values)
{
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, Percentage>) {
        double min{std::numeric_limits<double>::max()};
        for (const Percentage& value : values) {
            if (value.value < min) {
                min = value.value;
            }
        }
        return Percentage{min};
    } else {
        auto min{std::numeric_limits<T>::max()};
        for (const T& value : values) {
            if (value < min) {
                min = value;
            }
        }
        return {min};
    }
}

template <typename T>
static T get_max(const std::vector<T>& values)
{
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, Percentage>) {
        double max{std::numeric_limits<double>::lowest()};
        for (const Percentage& value : values) {
            if (value.value > max) {
                max = value.value;
            }
        }
        return Percentage{max};
    } else {
        auto max{std::numeric_limits<T>::lowest()};
        for (const T& value : values) {
            if (value > max) {
                max = value;
            }
        }
        return {max};
    }
}

const CompatibilityRules& get_compatibility_rules()
{
    static const CompatibilityRules result{
        {"brim_separation", CompatibilityRule::Average},
        {"dont_support_bridges", CompatibilityRule::IgnoreOverrides},
        {"travel_acceleration", CompatibilityRule::IgnoreOverrides},
        {"max_volumetric_extrusion_rate_slope_positive", CompatibilityRule::IgnoreOverrides},
        {"max_volumetric_extrusion_rate_slope_negative", CompatibilityRule::IgnoreOverrides},
        {"only_retract_when_crossing_perimeters", CompatibilityRule::IgnoreOverrides},
        {"raft_contact_distance", CompatibilityRule::Average},
        {"raft_expansion", CompatibilityRule::Max},
        {"raft_first_layer_density", CompatibilityRule::Average},
        {"raft_first_layer_expansion", CompatibilityRule::Max},
        {"gcode_resolution", CompatibilityRule::IgnoreOverrides},
        {"seam_position", CompatibilityRule::IgnoreOverrides},
        {"support_material_xy_spacing", CompatibilityRule::IgnoreOverrides},
        {"support_material_angle", CompatibilityRule::Average},
        {"support_material_contact_distance", CompatibilityRule::Average},
        {"support_material_bottom_contact_distance", CompatibilityRule::Average},
        {"support_material_enforce_layers", CompatibilityRule::Max},
        {"support_material_extrusion_width", CompatibilityRule::IgnoreOverrides},
        {"support_material_interface_contact_loops", CompatibilityRule::IgnoreOverrides},
        {"support_material_interface_layers", CompatibilityRule::Max},
        {"support_material_bottom_interface_layers", CompatibilityRule::Max},
        {"support_material_closing_radius", CompatibilityRule::Min},
        {"support_material_interface_spacing", CompatibilityRule::Min},
        {"support_material_interface_speed", CompatibilityRule::IgnoreOverrides},
        {"support_material_pattern", CompatibilityRule::IgnoreOverrides},
        {"support_material_interface_pattern", CompatibilityRule::IgnoreOverrides},
        {"support_material_spacing", CompatibilityRule::Min},
        {"support_material_speed", CompatibilityRule::Min},
        {"support_material_style", CompatibilityRule::IgnoreOverrides},
        {"support_material_synchronize_layers", CompatibilityRule::IgnoreOverrides},
        {"support_material_threshold", CompatibilityRule::IgnoreOverrides},
        {"support_material_with_sheath", CompatibilityRule::IgnoreOverrides},
        {"support_tree_angle", CompatibilityRule::Min},
        {"support_tree_angle_slow", CompatibilityRule::Min},
        {"support_tree_tip_diameter", CompatibilityRule::Min},
        {"support_tree_branch_diameter", CompatibilityRule::Min},
        {"support_tree_branch_diameter_angle", CompatibilityRule::IgnoreOverrides},
        {"support_tree_branch_diameter_double_wall", CompatibilityRule::IgnoreOverrides},
        {"support_tree_branch_distance", CompatibilityRule::Max},
        {"support_tree_top_rate", CompatibilityRule::IgnoreOverrides},
        {"thick_bridges", CompatibilityRule::IgnoreOverrides},
        {"travel_speed", CompatibilityRule::Min},
        {"travel_speed_z", CompatibilityRule::Min},
        {"travel_short_distance_acceleration", CompatibilityRule::Min},
        {"wipe_tower_extra_spacing", CompatibilityRule::Max},
        {"wipe_tower_extra_flow", CompatibilityRule::Max},
        {"wipe_tower_bridging", CompatibilityRule::Min},
        {"elefant_foot_compensation", CompatibilityRule::Average},
        {"min_feature_size", CompatibilityRule::IgnoreOverrides},
        {"min_bead_width", CompatibilityRule::IgnoreOverrides}
    };

    return result;
}

std::pair<ConfigValue, bool> apply_compatibility_rule(
    const ConfigValue* default_value,
    const std::vector<const ConfigItem*>& items,
    const std::vector<unsigned int>& extruder_candidates
)
{
    return apply_compatibility_rule(
        default_value,
        items,
        std::set<unsigned>{extruder_candidates.cbegin(), extruder_candidates.cend()}
    );
}

std::pair<ConfigValue, bool> apply_compatibility_rule(
    const ConfigValue* default_value,
    const std::vector<const ConfigItem*>& items,
    const std::set<unsigned>& extruder_candidates
)
{
    auto it{std::ranges::find_if(
        items,
        [](const ConfigItem* item) { return item != nullptr; }
    )};
    if (it == items.end()) {
        ASSERT(default_value);
        return {*default_value, false};
    }

    const ConfigItem& first_item{**it};

    if (!first_item.def().require_compatibility_rule) {
        return {*default_value, false};
    }
    const CompatibilityRule compatibility_rule{
        get_compatibility_rules().at(first_item.name())
    };

    if (!extruder_candidates.empty()) {
        bool all_same{true};
        for (unsigned extruder : extruder_candidates) {
            const ConfigItem* item{items[extruder]};
            if (item == nullptr && *ASSERT_VAL(default_value) != first_item.value()) {
                all_same = false;
                break;
            }
            if (item != nullptr && item->value() != first_item.value()) {
                all_same = false;
                break;
            }
        }
        if (all_same) {
            return {first_item.value(), false};
        }
    }

    if (compatibility_rule == CompatibilityRule::IgnoreOverrides) {
        ASSERT(default_value);
        return {*default_value, true};
    }

    return first_item.visit(
        [&](const auto& value) -> std::pair<ConfigValue, bool>
        {
            using Type = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<Type, int>
                          || std::is_same_v<Type, Percentage>
                          || std::is_same_v<Type, double>)
            {
                std::vector<Type> values;
                for (std::size_t tool_index{}; tool_index < items.size(); ++tool_index) {
                    const auto& item{items[tool_index]};
                    if (!extruder_candidates.empty() && !extruder_candidates.contains(tool_index)) {
                        continue;
                    }
                    if (item != nullptr) {
                        values.push_back(item->get<Type>());
                    } else {
                        values.push_back(default_value->get<Type>());
                    }
                }

                switch (compatibility_rule) {
                case CompatibilityRule::Average:
                    if constexpr (std::is_same_v<Type, double>
                                  || std::is_same_v<Type, Percentage>) {
                        return {ConfigValue{get_average(values)}, true};
                    } else {
                        PANIC("Average is only possible on doubles and percentages: " + first_item.def().name);
                        return {ConfigValue{0}, false};
                    }
                case CompatibilityRule::Min:
                    return {ConfigValue{get_min(values)}, true};
                case CompatibilityRule::Max:
                    return {ConfigValue{get_max(values)}, true};
                case CompatibilityRule::IgnoreOverrides:
                    PANIC("Ignore overrides should be handled before this");
                    return {ConfigValue{0}, false};
                case Slic3r::Domain::CompatibilityRule::Undefined:
                    PANIC("Undefined Compatibility rule in use");
                    return {ConfigValue{0}, false};
                }
            } else {
                PANIC("Invalid compatibility rule - only possible is Ignore overrides: " + first_item.def().name);
                return {ConfigValue{0}, false};
            }
            PANIC("Unreachble");
            return {ConfigValue{0}, false};
        }
    );
}

void SquashedConfig::add(
    const BoxRefs& boxes,
    const std::vector<unsigned>& extruder_candidates,
    const ConfigLocationSizes& location_sizes
)
{
    ASSERT(!boxes.empty());

    using It = std::vector<ConfigItem>::const_iterator;
    std::vector<It> begins;
    std::vector<It> ends;
    for (const auto& box : boxes) {
        begins.push_back(box.get().items.all_items().begin());
        ends.push_back(box.get().items.all_items().end());
    }

    const auto location_size_it{location_sizes.find(boxes.front().get().location)};
    ASSERT(location_size_it != location_sizes.end(), "Location size must be provided!");
    ASSERT(location_size_it->second == boxes.size());

    iterate_together(
        begins,
        ends,
        [&](const std::vector<ConfigItem>& items)
        {
            if (items.front().def().require_compatibility_rule) {
                const CompatibilityRule compatibility_rule{
                    get_compatibility_rules().at(items.front().name())
                };
                ASSERT(compatibility_rule != CompatibilityRule::IgnoreOverrides);

                std::vector<const ConfigItem*> item_pointers;
                std::ranges::transform(
                    items,
                    std::back_inserter(item_pointers),
                    [](const ConfigItem& item) { return &item; }
                );

                const auto [config_value, _]{
                    apply_compatibility_rule(nullptr, item_pointers, extruder_candidates)
                };

                m_values.insert({items.front().def().name, config_value});
            } else {
                m_values.insert({items.front().def().name, extract_values(items)});
            }
        }
    );

    using ConfigItemPointers = std::vector<const ConfigItem*>;
    std::map<std::string, ConfigItemPointers> to_apply_compatibility_rule;

    for (std::size_t i{}; i < boxes.size(); ++i) {
        const ConfigOverrides& overrides{boxes[i].get().overrides};
        for (const auto& item : overrides.overriden_items()) {
            const std::string& key{item.get().def().name};

            const bool is_vector{m_values.at(key).visit([](const auto& value) {
                using Type = std::remove_cvref_t<decltype(value)>;
                return Domain::is_std_vector_v<Type> || std::is_same_v<Type, EnumVectorWrapper>;
            })};

            if (is_vector) {
                item.get().visit([&](auto&& value){
                    using ValueType = std::remove_cvref_t<decltype(value)>;
                    if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
                        auto enum_vector_wrapper{m_values.at(key).get<EnumVectorWrapper>()};
                        std::vector<int> enum_values{enum_vector_wrapper.values()};
                        ASSERT(i < enum_values.size());
                        enum_values[i] = value.value();
                        const EnumVectorWrapper new_enum_vector_wrapper{
                            enum_values,
                            enum_vector_wrapper.type(),
                            enum_vector_wrapper.def()
                        };
                        m_values.at(key).set(new_enum_vector_wrapper);
                    } else if constexpr (
                        std::is_same_v<ValueType, EnumVectorWrapper>
                        || is_std_vector_v<ValueType>
                    ) {
                        PANIC("Vector of vectors is not supported!");
                    } else {
                        auto values{m_values.at(key).get<std::vector<ValueType>>()};
                        ASSERT(i < values.size());
                        values[i] = value;
                        m_values.at(key).set(values);
                    }
                });
            } else {
                ASSERT(item.get().def().require_compatibility_rule);
                const auto it{to_apply_compatibility_rule.find(key)};
                if (it == to_apply_compatibility_rule.end()) {
                    to_apply_compatibility_rule[key] = ConfigItemPointers(boxes.size());
                }
                to_apply_compatibility_rule.at(key)[i] = &item.get();
            }
        }
    }

    for (const auto& [key, values] : to_apply_compatibility_rule) {
        m_values.at(key).set(
            apply_compatibility_rule(&m_values.at(key), values, extruder_candidates).first
        );
    }
}

const std::vector<std::string>& FullConfig::keys() const {
    return m_keys;
}

FullConfig::FullConfig(
    const BoxOrBoxesVector& input,
    const std::vector<unsigned>& extruder_candidates,
    const ConfigLocationSizes& location_sizes
) :
    SquashedConfig{input, extruder_candidates, location_sizes}
{
    std::ranges::transform(
        m_values,
        std::back_inserter(m_keys),
        [](const auto& pair) { return pair.first; }
    );
}

const ConfigValue& FullConfig::get_value(const std::string& key) const {
    const auto it{m_values.find(key)};
    ASSERT(it != m_values.end(), "The key '" + key + "' is not part of this config.");
    return it->second;
}

PartialConfig::PartialConfig(
    const BoxOrBoxesVector& input,
    const ConfigLocationSizes& location_sizes
) :
    SquashedConfig{input, {}, location_sizes}
{}

std::optional<ConfigValue> PartialConfig::get_value(const std::string& key) const {
    const auto it{m_values.find(key)};
    if (it == m_values.end()) {
        return std::nullopt;
    }
    return it->second;
}

namespace {
    template<typename T>
    std::size_t hash(const T& value) {
        if constexpr(
            !std::is_same_v<T, std::string>
            && !std::is_same_v<T, std::vector<bool>>
            && std::ranges::range<T>
        ) {
            std::size_t seed = 0;
            for (const auto& v : value) {
                boost::hash_combine(seed, hash(v));
            }
            return seed;
        } else {
            return std::hash<T>{}(value);
        }
    }
    std::size_t hash(const ConfigValue& value) {
        return value.visit([](auto&& value){
            using ValueType = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, EnumVectorWrapper>) {
                const EnumVectorWrapper& wrapped_enums{value};
                return hash(wrapped_enums.values());
            } else if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
                const EnumWrapper& wrapped_enum{value};
                return hash(wrapped_enum.value());
            } else if constexpr (std::is_same_v<ValueType, std::vector<EnumWrapper>>) {
                std::vector<int> values;
                for (const EnumWrapper& wrapped_enum : value) {
                    values.push_back(wrapped_enum.value());
                }
                return hash(values);
            } else {
                return hash(value);
            }
        });
    }
}

std::size_t SquashedConfig::hash() const {
    std::size_t seed{0};
    for (const auto& [key, value] : m_values) {
        boost::hash_combine(seed, Domain::hash(key));
        boost::hash_combine(seed, Domain::hash(value));
    }
    return seed;
}

ConfigView::ConfigView(FullConfigPtr full_config, const std::vector<PartialConfigPtr>& partial_configs)
    : m_full_config{full_config}, m_partial_configs{partial_configs}
{
    std::vector<std::string> partial_config_keys;
    ASSERT(m_full_config);
    for (const PartialConfigPtr& ptr : m_partial_configs) {
        ASSERT(ptr);
        for (const auto&[key, _] : ptr->m_values) {
            partial_config_keys.push_back(key);
        }
    }

    m_keys = m_full_config->keys();
    m_keys.insert(m_keys.end(), partial_config_keys.begin(), partial_config_keys.end());
    std::ranges::sort(m_keys);
    m_keys.erase(std::unique(m_keys.begin(), m_keys.end()), m_keys.end());
}

bool ConfigView::operator==(const ConfigView& other) const {
    if (m_partial_configs.size() != other.m_partial_configs.size()) {
        return false;
    }
    for (std::size_t i{}; i < m_partial_configs.size(); ++i) {
        if (m_partial_configs[i] != other.m_partial_configs[i]) {
            return false;
        }
    }

    return *m_full_config == *other.m_full_config;
}

std::size_t ConfigView::hash() const {
    std::size_t seed{m_full_config->hash()};
    for (const PartialConfigPtr& partial_config : m_partial_configs) {
        boost::hash_combine(seed, partial_config->hash());
    }
    return seed;
}

std::vector<std::string> ConfigView::diff_keys(const ConfigView& other) const
{
    std::vector<std::string> result;
    for (const std::string& key : m_keys) {
        if (get_value(key) != other.get_value(key)) {
            result.push_back(key);
        }
    }

    return result;
}

ConfigValue ConfigView::get_value(const std::string& key) const {
    for (auto rev_it = m_partial_configs.rbegin(); rev_it != m_partial_configs.rend(); ++rev_it) {
        const PartialConfigPtr& partial_config{*rev_it};
        if (auto result{partial_config->get_value(key)}) {
            return *result;
        }
    }
    return m_full_config->get_value(key);
}

} // namespace Slic3r::Domain
