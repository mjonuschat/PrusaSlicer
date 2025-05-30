#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <algorithm>
#include <set>

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

ConfigItem::ConfigItem(const ConfigItemDef& def, std::string_view box_type)
    : m_name{def.name}, m_is_nullable{false}, m_def{&def}, m_data{def.init_fn ? def.init_fn() : def.init_fn_ex(box_type)}
{
    m_is_nullable = (std::ranges::find(def.overrides_in, box_type) != def.overrides_in.end());
    if (m_is_nullable)
        set_null(true); // Overrides are null by default.
}

void ConfigItem::set_null(bool null)
{
    ASSERT(m_is_nullable);
    m_is_null = null;
}

bool ConfigItem::is_null() const
{
    return m_is_null;
}

bool ConfigItem::is_nullable() const { return m_is_nullable; }
const std::string& ConfigItem::name() const { return m_name; }

namespace Impl {
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
}

std::size_t ConfigItem::hash() const {
    if (m_is_null) {
        return 0;
    }

    return m_data.visit([](auto&& value){
        using ValueType = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<ValueType, EnumVectorWrapper>) {
            const EnumVectorWrapper& wrapped_enums{value};
            return Impl::hash(wrapped_enums.values());
        } else if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            const EnumWrapper& wrapped_enum{value};
            return Impl::hash(wrapped_enum.value());
        } else if constexpr (std::is_same_v<ValueType, std::vector<EnumWrapper>>) {
            std::vector<int> values;
            for (const EnumWrapper& wrapped_enum : value) {
                values.push_back(wrapped_enum.value());
            }
            return Impl::hash(values);
        } else {
            return Impl::hash(value);
        }
    });
}

ConfigItem& ConfigBox::opt(const std::string_view key) {
    auto it = std::lower_bound(m_items.begin(), m_items.end(), key,
        [](const ConfigItem& i, const auto& val) { return i.name() < val; });
    if (it != m_items.end() && it->name() == key)
        return *it;
    PANIC("Option not found", key);
    throw std::exception(); // to silence a warning
}

std::optional<const ConfigItem*> ConfigBox::contains(const std::string_view key) const {
    auto it = std::lower_bound(m_items.begin(), m_items.end(), key,
        [](const ConfigItem& i, const auto& val) { return i.name() < val; });
    if (it == m_items.end() || it->name() != key)
        return std::nullopt;
    return std::make_optional(&(*it));
}



std::vector<std::string> ConfigBox::diff_keys(const ConfigBox& other) const
{
    ASSERT(this->type() == other.type());
    std::vector<std::string> out;
    for (size_t i = 0; i < m_items.size(); ++i)
        if (m_items[i] != other.m_items[i])
            out.emplace_back(m_items[i].name());
    return out;
}


std::size_t ConfigBox::hash() const
{
    std::size_t seed{0};
    for (const ConfigItem& item : m_items) {
        boost::hash_combine(seed, item.hash());
    }
    return seed;
}

ConfigBox::ConfigBox(const ConfigDefinitions& defs, std::string_view type)
: m_type{type}
{
    for (const ConfigItemDef& def : defs.defs()) {
        if (def.location == type || std::ranges::find(def.overrides_in, type) != def.overrides_in.end())
            m_items.emplace_back(ConfigItem(def, type));
    }
}

FullConfig::FullConfig(const FullConfigInput& all_boxes) {
    for (const auto& box_or_boxes : all_boxes) {
        std::visit([this](auto&& box_or_boxes) { this->add(box_or_boxes); }, box_or_boxes);
    }

    for (const auto& [key, _] : m_single_items) {
        m_single_item_keys.push_back(key);
    }

    for (const auto &[key, _] : m_multi_items) {
        m_multi_item_keys.push_back(key);
    }
}

void FullConfig::add(const ConfigBox& box)
{
    for (const ConfigItem& item : box) {
        if (auto it_m = m_multi_items.find(item.name()); it_m != m_multi_items.end() && ! item.is_null())
            it_m->second = std::vector<ConfigItem>(it_m->second.size(), item);
        else {
            auto it_s = m_single_items.find(item.name());
            if (it_s == m_single_items.end() || !item.is_null())
                m_single_items.emplace(item.name(), item);
        }
    }
}



void FullConfig::add(const BoxRefs& boxes)
{
    std::set<std::string> box_types;

    for (size_t box_id=0; box_id<boxes.size(); ++box_id) {
        const ConfigBox& box = boxes[box_id];
        box_types.insert(std::string(box.type()));

        for (const ConfigItem& item : box) {
            if (auto it_s = m_single_items.find(item.name()); it_s != m_single_items.end()) {
                // This item is already in the single list. Apparently there are overrides at the extruder level.
                // Move it to the multi list and copy the value for all extruders for now.
                ASSERT(m_multi_items.find(item.name()) == m_multi_items.end());
                m_multi_items.emplace(item.name(), std::vector<ConfigItem>(boxes.size(), it_s->second));
                m_single_items.erase(it_s);
            }

            auto it_m = m_multi_items.find(item.name());
            if (it_m == m_multi_items.end()) {
                // The item is not in the multi list yet. Default construct an empty vector for it.
                it_m = m_multi_items.emplace(item.name(), std::vector<ConfigItem>()).first;
            }

            // Right now, the vector is there and pointed to by it_m.
            // It may not have all the elements though.

            if (box_id < it_m->second.size()) {
                if (!item.is_null()) {
                    // The element is already there, and we should override it.
                    it_m->second[box_id] = item;
                } else {
                    // No action needed. This does not override.
                }
            } else {
                // Element is not there. Insert it.
                ASSERT(box_id == it_m->second.size()); // Previous element should be there by now.
                ASSERT(! item.is_null()); // It should never be null in this case.
                it_m->second.emplace_back(item);
            }
        }
    }
    ASSERT(box_types.size() == 1, "Only boxes of the same type can be added in a vector like this.");
    ASSERT(std::all_of(m_multi_items.begin(), m_multi_items.end(),
        [this](const auto& pair) {
            return pair.second.size() == m_multi_items.begin()->second.size();
        }
    ), "All vectors in the multi list must have the same size.");
}

std::vector<std::string> ConfigView::diff_keys(const ConfigView& other) const
{
    std::vector<std::string> result;
    for (const std::string& key : m_full_config->m_single_item_keys) {
        if (opt_single(key) != other.opt_single(key)) {
            result.push_back(key);
        }
    }

    for (const std::string& key : m_full_config->m_multi_item_keys) {
        if (opt_multi(key) != other.opt_multi(key)) {
            result.push_back(key);
        }
    }

    return result;
}

bool ConfigView::operator==(const ConfigView& other) const {
    return diff_keys(other).empty();
}

std::size_t ConfigView::hash() const {
    std::size_t seed{m_full_config->hash()};
    for (const ConfigBoxPtr& box : m_config_boxes) {
        boost::hash_combine(seed, box->hash());
    }
    return seed;
}

std::size_t FullConfig::hash() const {
    std::size_t seed{std::hash<std::string>{}(std::string{this->name()})};

    for (const auto& [_, item] : m_single_items) {
        boost::hash_combine(seed, item.hash());
    }
    for (const auto& [_, items] : m_multi_items) {
        for (const ConfigItem& item : items) {
            boost::hash_combine(seed, item.hash());
        }
    }
    return seed;
}

const ConfigItem& FullConfig::opt_single(const std::string_view key) const {
    auto it = m_single_items.find(std::string(key));
    ASSERT(it != m_single_items.end(), "Key '" + std::string{key} +"' does not exist!");
    return it->second;
}

const std::vector<ConfigItem>& FullConfig::opt_multi(const std::string_view key) const {
    const auto it{m_multi_items.find(std::string(key))};
    ASSERT(it != m_multi_items.end(), "Key '" + std::string{key} +"' does not exist!");
    return it->second;
}
} // namespace Slic3r::Domain
