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

ConfigDefinitions::ConfigDefinitions(const std::vector<std::string>& acceptable_boxes, std::function<void(ConfigDefinitions&)> init_fn)
{
    m_acceptable_boxes = acceptable_boxes;
    init_fn(*this);
    std::sort(m_defs.begin(), m_defs.end());
    this->check_valid();
    m_finalized = true;
}


ConfigItemDef* ConfigDefinitions::add(const std::string_view name, ConfigItemType type)
{
    ASSERT(!m_finalized);
    return &m_defs.emplace_back(ConfigItemDef{std::string(name), type});
}


void ConfigDefinitions::check_valid() const
 {
    ASSERT(std::is_sorted(m_defs.begin(), m_defs.end()));
    ASSERT(std::adjacent_find(m_defs.begin(), m_defs.end(), // check for duplicates
        [](const auto& a, const auto& b) { return a.name == b.name; }) == m_defs.end());

    for (const ConfigItemDef& def : m_defs) {
        ASSERT(def.type != ConfigItemType::None);
        ASSERT(! def.location.empty());
        ASSERT(std::ranges::find(def.overrides_in, def.location) == def.overrides_in.end());
        ASSERT(int(bool(def.init_fn)) ^ int(bool(def.init_fn_ex)));
        ASSERT(def.type == ConfigItemType::Enum || def.type == ConfigItemType::Enums || def.enum_values.empty());
        ASSERT((def.type != ConfigItemType::Enum && def.type != ConfigItemType::Enums) || !def.enum_values.empty());
        ASSERT(def.type == ConfigItemType::Enum || def.type == ConfigItemType::Enums || ! def.enum_type.has_value());
        ASSERT((def.type != ConfigItemType::Enum && def.type != ConfigItemType::Enums) || def.enum_type.has_value());
        ASSERT(std::is_sorted(def.enum_values.begin(), def.enum_values.end()));
        ASSERT(std::adjacent_find(def.enum_values.begin(), def.enum_values.end(), // check for duplicates
        [](const auto& a, const auto& b) { return a.enum_value == b.enum_value; }) == def.enum_values.end());

        // Check that all items are assigned to valid boxes.
        ASSERT(std::any_of(m_acceptable_boxes.begin(), m_acceptable_boxes.end(), [&def](const auto& b) { return def.location == b; }));
        ASSERT(std::all_of(def.overrides_in.begin(), def.overrides_in.end(), [this](const auto& box) {
            return std::any_of(m_acceptable_boxes.begin(), m_acceptable_boxes.end(), [&box](const auto& b) { return box == b; });
        }));

        // Check that all choices (if used) have the same key type and that it matches the item type.
        if (!def.choices.empty()) {
            for (const auto& [value, str] : def.choices) {
                ASSERT((def.type == ConfigItemType::String && std::holds_alternative<std::string>(value))
                    || (def.type == ConfigItemType::Int && std::holds_alternative<int>(value))
                    || (def.type == ConfigItemType::Double && std::holds_alternative<double>(value))
                    || (def.type == ConfigItemType::Percent && std::holds_alternative<double>(value))
                    || (def.type == ConfigItemType::FloatOrPercent && std::holds_alternative<double>(value)));
            }
        }
        

    }
}



ConfigItem::ConfigItem(const ConfigItemDef& def, std::string_view box_type)
    : m_def{ &def },
        m_type{ def.type  },
        m_data{ std::any()},
        m_is_nullable{ false },
        m_name{ def.name } {
        // Read the def and create the respective object to store the payload.
        switch (def.type) {
        case ConfigItemType::Bool    :        m_data = bool();                     break;
        case ConfigItemType::Int     :        m_data = int();                      break;
        case ConfigItemType::IntOptional :    m_data = std::optional<int>();       break;
        case ConfigItemType::Double  :        m_data = double();                   break;
        case ConfigItemType::String  :        m_data = std::string();              break;
        case ConfigItemType::Enum    :        m_data = int();                      break;
        case ConfigItemType::Point   :        m_data = Vec2d();                    break;
        case ConfigItemType::Percent :        m_data = Percentage();               break;
        case ConfigItemType::FloatOrPercent : m_data = FloatOrPercentage();        break;
        case ConfigItemType::Bools   :        m_data = std::vector<bool>();        break;
        case ConfigItemType::Enums   :        [[fallthrough]];
        case ConfigItemType::Ints    :        m_data = std::vector<int>();         break;
        case ConfigItemType::Doubles :        m_data = std::vector<double>();      break;
        case ConfigItemType::Strings :        m_data = std::vector<std::string>(); break;
        case ConfigItemType::Points  :        m_data = std::vector<Vec2d>();       break;
        default : PANIC();
        }
        m_is_nullable = (std::ranges::find(def.overrides_in, box_type) != def.overrides_in.end());
        if (m_is_nullable)
            set_null(true); // Overrides are null by default.
        if (def.init_fn)
            def.init_fn(*this);
        else
            def.init_fn_ex(*this, box_type);
}



ConfigItem::ConfigItem(const ConfigItem& other)
: m_name(other.m_name),
    m_type(other.m_type),
    m_is_nullable(other.m_is_nullable),
    m_is_null(other.m_is_null),
    m_def(other.m_def),
    m_data(other.m_data)
{}



ConfigItem& ConfigItem::operator=(const ConfigItem& other) {
    if (this == &other)
        return *this;  // Self-assignment check
    m_data = other.m_data;
    m_type = other.m_type;
    m_is_nullable = other.m_is_nullable;
    m_is_null = other.m_is_null;
    m_def = other.m_def;
    m_name = other.m_name;
    return *this;
}

bool ConfigItem::operator==(const ConfigItem& other) const
{
    if (this->type() != other.type()
     || this->is_vector() != other.is_vector()
     || this->name() != other.name())
        return false;

    if (this->m_is_null != other.m_is_null)
        return false;
    if (this->type() == ConfigItemType::Enum && this->def().enum_type.type() != other.def().enum_type.type())
        return false;
    if (this->m_is_nullable && this->is_null())
        return true; // Both are set to null.

    using CIT = ConfigItemType;
    switch (this->type()) {
    case CIT::Bool: return this->get<bool>() == other.get<bool>();
    case CIT::Int: return this->get<int>() == other.get<int>();
    case CIT::IntOptional: return this->get<std::optional<int>>() == other.get<std::optional<int>>();
    case CIT::Double: return this->get<double>() == other.get<double>();
    case CIT::String: return this->get<std::string>() == other.get<std::string>();
    case CIT::Enum: return this->get_enum_as_int() == other.get_enum_as_int();
    case CIT::Percent: return this->get<Percentage>() == other.get<Percentage>();
    case CIT::FloatOrPercent: return this->get<FloatOrPercentage>() == other.get<FloatOrPercentage>();
    case CIT::Point: return this->get<Vec2d>() == other.get<Vec2d>();
    case CIT::Bools: return this->get<std::vector<bool>>() == other.get<std::vector<bool>>();
    case CIT::Enums: [[fallthrough]];
    case CIT::Ints: return this->get<std::vector<int>>() == other.get<std::vector<int>>();
    case CIT::Doubles: return this->get<std::vector<double>>() == other.get<std::vector<double>>();
    case CIT::Strings: return this->get<std::vector<std::string>>() == other.get<std::vector<std::string>>();
    case CIT::Points: return this->get<std::vector<Vec2d>>() == other.get<std::vector<Vec2d>>();
    default : PANIC();
    }

    PANIC();
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



template <IsNotEnumOrVectorOfEnums T>
const T& ConfigItem::get() const
{
    ASSERT(!m_is_null, "ConfigItem '" + std::string{m_name} + " 'is null");
    try {
        const T* value = std::any_cast<T>(&m_data);
        ASSERT(value, "Type of '" + m_name + "' does not match");
        return *value;
    } catch (const std::bad_any_cast&) {
        PANIC("Type of '" + m_name + "' does not match");
    }
    throw std::exception(); // silence warning
}



template<IsNotEnumOrVectorOfEnums T>
void ConfigItem::set(const T& value)
{
    if (m_data.type() == typeid(T))
        m_data = std::make_any<T>(value);
    else
        PANIC("Type of '" + m_name + "' does not match");
}



void ConfigItem::set_enum_from_string(std::string_view value) {
    ASSERT(m_type == ConfigItemType::Enum);
    for (const EnumValueDef& evd : def().enum_values) {
        if (evd.str_serialized == value) {
            m_data = std::make_any<int>(int(evd.enum_value));
            return;
        }
    }
    PANIC();
}

std::pair<std::string_view, std::string_view> ConfigItem::get_enum_strings() const {
    for (const EnumValueDef& evd : def().enum_values)
        if (evd.enum_value == std::any_cast<int>(m_data))
            return std::make_pair(std::string_view(evd.str_serialized), std::string_view(evd.str_ui));
    PANIC();
    throw std::exception();
}

void ConfigItem::set_enums_from_strings(std::vector<std::string> values)
{
    ASSERT(m_type == ConfigItemType::Enums);
    std::vector<int> payload(values.size());

    size_t i = 0;
    bool ok = false;
    for (const std::string& value : values) {
        for (const EnumValueDef& evd : def().enum_values) {
            if (evd.str_serialized == value) {
                payload[i] = evd.enum_value;
                ok = true;
                break;
            }
        }
        ASSERT(ok);
        ++i;
    }
    set_enums_from_ints(payload);
}

std::vector<std::pair<std::string_view, std::string_view>> ConfigItem::get_enums_strings() const
{
    ASSERT(m_type == ConfigItemType::Enums);
    auto payload = get_enums_as_ints();
    std::vector<std::pair<std::string_view, std::string_view>> out;
    for (int value : payload) {
        auto it = std::ranges::find_if(def().enum_values,
            [value](const Domain::EnumValueDef& evd) { return evd.enum_value == value; });
        ASSERT(it != def().enum_values.end());
        out.emplace_back(std::make_pair(it->str_serialized, it->str_ui));
    }
    return out;
}

void ConfigItem::set_enum_from_int(int value)
{
    // No check for type here, this is private method and the check is upstack.
    ASSERT(std::find_if(def().enum_values.begin(), def().enum_values.end(),
        [value](const EnumValueDef& evd) { return evd.enum_value == value; }) != def().enum_values.end());
    m_data = std::make_any<int>(int(value));
}

int ConfigItem::get_enum_as_int() const
{
    // No check here, this is private method and the check is upstack.
    return std::any_cast<int>(m_data);
}

void ConfigItem::set_enums_from_ints(const std::vector<int>& values)
{
    // No check for type here, this is private method and the check is upstack.
    for (int value : values)
        ASSERT(std::find_if(def().enum_values.begin(), def().enum_values.end(),
            [value](const EnumValueDef& evd) { return evd.enum_value == value; }) != def().enum_values.end());
    m_data = std::make_any<std::vector<int>>(values);
}

std::vector<int> ConfigItem::get_enums_as_ints() const
{
    // No check here, this is private method and the check is upstack.
    return std::any_cast<std::vector<int>>(m_data);
}


template<class T>
std::vector<T>& ConfigItem::vec()
{
    ASSERT(is_vector());
    try {
        std::vector<T>* vec_ptr = std::any_cast<std::vector<T>>(&m_data);
        ASSERT(vec_ptr, "Type does not match.", vec_ptr, type());
        return *vec_ptr;
    } catch (const std::bad_any_cast&) {}
    PANIC("Type does not match.", this->type());
}


namespace Impl {
template<typename T>
std::size_t hash(const T& value) {
    if constexpr(! std::is_same_v<T, std::string> && ! std::is_same_v<T, std::vector<bool>>
                 && std::ranges::range<T>) {
        // Both std::string and std::vector<bool> have default hash functions. The latter
        // is even impossible to hash element-wise as it is an abomination which returns
        // some unhashable proxy type.
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

    using CIT = ConfigItemType;
    using Impl::hash;
    switch (m_def->type) {
        case CIT::Bool: return hash(this->get<bool>());
        case CIT::Int: return hash(this->get<int>());
        case CIT::IntOptional: return hash(this->get<std::optional<int>>());
        case CIT::Double: return hash(this->get<double>());
        case CIT::String: return hash(this->get<std::string>());
        case CIT::Enum: return hash(this->get_enum_as_int());
        case CIT::Point: return hash(this->get<Vec2d>());
        case CIT::FloatOrPercent: return hash(this->get<FloatOrPercentage>());
        case CIT::Percent: return hash(this->get<Percentage>());
        case CIT::Bools: return hash(this->get<std::vector<bool>>());
        case CIT::Ints: return hash(this->get<std::vector<int>>());
        case CIT::Doubles: return hash(this->get<std::vector<double>>());
        case CIT::Strings: return hash(this->get<std::vector<std::string>>());
        case CIT::Points: return hash(this->get<std::vector<Vec2d>>());
        case CIT::Enums: return hash(this->get_enums_as_ints());
        case CIT::None: PANIC("None ConfigItem can't be hashed!");
        default: PANIC("Unhandled ConfigItem");
    }
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

// Explicit instantiations for getters.
template const bool& ConfigItem::get<bool>() const;
template const int& ConfigItem::get<int>() const;
template const double& ConfigItem::get<double>() const;
template const Percentage& ConfigItem::get<Percentage>() const;
template const FloatOrPercentage& ConfigItem::get<FloatOrPercentage>() const;
template const std::string& ConfigItem::get<std::string>() const;
template const Vec2d& ConfigItem::get<Vec2d>() const;
template const std::optional<int>& ConfigItem::get<std::optional<int>>() const;
template const std::vector<bool>& ConfigItem::get<std::vector<bool>>() const;
template const std::vector<int>& ConfigItem::get<std::vector<int>>() const;
template const std::vector<double>& ConfigItem::get<std::vector<double>>() const;
template const std::vector<std::string>& ConfigItem::get<std::vector<std::string>>() const;
template const std::vector<Vec2d>& ConfigItem::get<std::vector<Vec2d>>() const;
template const std::vector<Percentage>& ConfigItem::get<std::vector<Percentage>>() const;
template const std::vector<FloatOrPercentage>& ConfigItem::get<std::vector<FloatOrPercentage>>() const;
template const std::vector<std::optional<int>>& ConfigItem::get<std::vector<std::optional<int>>>() const;

// Explicit instantiations for setters.
template void ConfigItem::set(const bool&);
template void ConfigItem::set(const int&);
template void ConfigItem::set(const double&);
template void ConfigItem::set(const Percentage&);
template void ConfigItem::set(const FloatOrPercentage&);
template void ConfigItem::set(const std::string&);
template void ConfigItem::set(const Vec2d&);
template void ConfigItem::set(const std::optional<int>&);
template void ConfigItem::set(const std::nullopt_t&);
template void ConfigItem::set(const std::vector<bool>&);
template void ConfigItem::set(const std::vector<int>&);
template void ConfigItem::set(const std::vector<double>&);
template void ConfigItem::set(const std::vector<std::string>&);
template void ConfigItem::set(const std::vector<Vec2d>&);

// And the respective explicit instantiations.
template std::vector<bool>& ConfigItem::vec();
template std::vector<int>& ConfigItem::vec();
template std::vector<double>& ConfigItem::vec();
template std::vector<std::string>& ConfigItem::vec();

} // namespace Slic3r::Domain
