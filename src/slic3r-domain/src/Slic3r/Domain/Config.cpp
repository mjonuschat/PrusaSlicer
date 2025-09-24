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
    : m_value{def.init_fn ? def.init_fn() : def.init_fn_ex(location)}, m_def{&def}
{}

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

ConfigItem* ConfigItems::contains(const std::string& key) {
    return find_item(m_items, key);
}

const ConfigItem* ConfigItems::contains(const std::string& key) const {
    return find_item(m_items, key);
}

const std::vector<ConfigItem> &ConfigItems::all_items() const
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
    const auto item_index{find(key)};
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

ConfigItem* ConfigOverrides::contains(const std::string& key) {
    return find_item(m_items, key);
}
const ConfigItem* ConfigOverrides::contains(const std::string& key) const {
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

std::vector<std::string> ConfigOverrides::diff_overriden_keys(const ConfigOverrides& other) const
{
    return diff_keys<std::string, std::size_t>(
        m_used_overrides,
        other.m_used_overrides,
        [&](std::size_t index_a, std::size_t index_b)
        { return m_items.at(index_a) == other.m_items.at(index_b); }
    );
}

std::size_t ConfigOverrides::find(const std::string& key) {
    const auto index{find_item_index(m_items, key)};
    ASSERT(index, "The key does not belong to this!");
    return *index;
}

ContainsResult ConfigBox::contains(const std::string& key) {
    if (auto* item{overrides.contains(key)}) {
        return {item, true};
    }
    return {items.contains(key), false};
}

ConstContainsResult ConfigBox::contains(const std::string& key) const {
    if (auto* item{overrides.contains(key)}) {
        return {item, true};
    }
    return {items.contains(key), false};
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
    const BoxOrBoxesVector& all_boxes, const ConfigLocationSizes& location_sizes
)
{
    for (const auto& box_or_boxes : all_boxes) {
        std::visit([&](auto&& box_or_boxes) {
            using ValueType = std::remove_cvref_t<decltype(box_or_boxes)>;
            if constexpr (std::is_same_v<ValueType, BoxRef>) {
                this->add(box_or_boxes.get(), location_sizes);
            } else if constexpr (std::is_same_v<ValueType, BoxRefs>) {
                this->add(box_or_boxes, location_sizes);
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

void SquashedConfig::add(const BoxRefs& boxes, const ConfigLocationSizes& location_sizes)
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

    iterate_together(begins, ends, [&](const std::vector<ConfigItem>& items){
        m_values.insert({items.front().def().name, extract_values(items)});
    });

    for (std::size_t i{}; i < boxes.size(); ++i) {
        const ConfigOverrides& overrides{boxes[i].get().overrides};
        for (const auto& item : overrides.overriden_items()) {
            const std::string& key{item.get().def().name};
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
        }
    }
}

const std::vector<std::string>& FullConfig::keys() const {
    return m_keys;
}

FullConfig::FullConfig(const BoxOrBoxesVector& input, const ConfigLocationSizes& location_sizes): SquashedConfig{input, location_sizes} {
    std::ranges::transform(m_values, std::back_inserter(m_keys), [](const auto& pair) {
        return pair.first;
    });
}

const ConfigValue& FullConfig::get_value(const std::string& key) const {
    const auto it{m_values.find(key)};
    ASSERT(it != m_values.end(), "The key '" + key + "' is not part of this config.");
    return it->second;
}

PartialConfig::PartialConfig(const BoxOrBoxesVector& input, const ConfigLocationSizes& location_sizes)
    : SquashedConfig{input, location_sizes}
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
    ASSERT(m_full_config);
    for (const PartialConfigPtr& ptr : m_partial_configs) {
        ASSERT(ptr);
    }
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

    for (const std::string& key : m_full_config->keys()) {
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
