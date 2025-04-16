#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Assert.hpp"

#include "ConfigItemValue.hpp"

#include <algorithm>

using namespace Slic3r::Domain::detail;

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
        ASSERT(! def.belongs_to.empty() || ! def.belongs_to_optional.empty());
        ASSERT(int(bool(def.init_fn)) ^ int(bool(def.init_fn_ex)));
        ASSERT(def.type == ConfigItemType::Enum || def.enum_values.empty());
        ASSERT(def.type != ConfigItemType::Enum || ! def.enum_values.empty());
        ASSERT(def.type == ConfigItemType::Enum || ! def.enum_type.has_value());
        ASSERT(def.type != ConfigItemType::Enum || def.enum_type.has_value());
        ASSERT(std::is_sorted(def.enum_values.begin(), def.enum_values.end()));
        ASSERT(std::adjacent_find(def.enum_values.begin(), def.enum_values.end(), // check for duplicates
        [](const auto& a, const auto& b) { return a.enum_value == b.enum_value; }) == def.enum_values.end());

        // Check that all items are assigned to valid boxes.
        ASSERT(std::all_of(def.belongs_to.begin(), def.belongs_to.end(), [this](const auto& box) {
            return std::any_of(m_acceptable_boxes.begin(), m_acceptable_boxes.end(), [&box](const auto& b) { return box == b; });
        })); 
        ASSERT(std::all_of(def.belongs_to_optional.begin(), def.belongs_to_optional.end(), [this](const auto& box) {
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
        m_type{ def.type },
        m_data{ nullptr },
        m_is_nullable{ false },
        m_name{ def.name } {
        // Read the def and create the respective polymorphic object to store the payload.
        switch (def.type) {
        case ConfigItemType::Bool    : m_data = new ConfigItemValueBool;    break;
        case ConfigItemType::Int     : m_data = new ConfigItemValueInt;     break;
        case ConfigItemType::Double  : m_data = new ConfigItemValueDouble;  break;
        case ConfigItemType::String  : m_data = new ConfigItemValueString;  break;
        case ConfigItemType::Enum    : m_data = new ConfigItemValueEnum;    break;
        case ConfigItemType::Percent : m_data = new ConfigItemValuePercent; break;
        case ConfigItemType::FloatOrPercent : m_data = new ConfigItemValueFloatOrPercent; break;
        case ConfigItemType::Bools   : m_data = new ConfigItemValueBools;   break;
        case ConfigItemType::Ints    : m_data = new ConfigItemValueInts;    break;
        case ConfigItemType::Doubles : m_data = new ConfigItemValueDoubles; break;
        case ConfigItemType::Strings : m_data = new ConfigItemValueStrings; break;
        default : PANIC();
        }
        m_is_nullable = std::any_of(def.belongs_to_optional.begin(), def.belongs_to_optional.end(),
            [&box_type](const auto& t) { return box_type == t; });
        if (m_is_nullable)
            set_null(true); // nullables are null by default.
        if (def.init_fn)
            def.init_fn(*this);
        else
            def.init_fn_ex(*this, box_type);
}



ConfigItem::~ConfigItem() {
    delete m_data;
}


ConfigItem::ConfigItem(const ConfigItem& other)
: m_def(other.m_def),
    m_type(other.m_type),
    m_is_nullable(other.m_is_nullable),
    m_data(other.m_data->clone()),
    m_name(other.m_name)
{}



ConfigItem& ConfigItem::operator=(const ConfigItem& other) {
    if (this == &other)
        return *this;  // Self-assignment check
    delete m_data;
    m_data = other.m_data->clone();
    m_type = other.m_type;
    m_is_nullable = other.m_is_nullable;
    m_def = other.m_def;
    m_name = other.m_name;
    return *this;
}



bool ConfigItem::operator==(const ConfigItem& other) const
{
    if (this->type() != other.type()
     || this->m_is_nullable != other.m_is_nullable
     || this->is_vector() != other.is_vector())
        return false;
    if (this->m_is_nullable && this->is_null() != other.is_null())
        return false;
    if (this->is_null())
        return true;
    if (this->type() == ConfigItemType::Enum && this->def().enum_type.type() != other.def().enum_type.type())
        return false;
    if (this->type() == ConfigItemType::FloatOrPercent) {
        if (this->is_percent() != other.is_percent())
            return false;
    }

    switch (this->type()) {
    case ConfigItemType::Bool    : return get<bool>() == other.get<bool>();
    case ConfigItemType::Int     : return get<int>() == other.get<int>();
    case ConfigItemType::String  : return get<std::string>() == other.get<std::string>();
    case ConfigItemType::Enum    : return get_enum_as_int() == other.get_enum_as_int();
    case ConfigItemType::Double  : return get<double>() == other.get<double>();
    case ConfigItemType::FloatOrPercent: return (this->is_percent() ? double(get<Percentage>()) == double(other.get<Percentage>()) : get<double>() == other.get<double>());
    case ConfigItemType::Percent : return double(get<Percentage>()) == double(other.get<Percentage>());
    case ConfigItemType::Bools   : return vec<bool>() == other.vec<bool>();
    case ConfigItemType::Ints    : return vec<int>() == other.vec<int>();
    case ConfigItemType::Doubles : return vec<double>() == other.vec<double>();
    case ConfigItemType::Strings : return vec<std::string>() == other.vec<std::string>();
    }
    PANIC();
}



void ConfigItem::set_null(bool null)
{
    ASSERT(m_is_nullable);
    m_data->set_null(null);
}

bool ConfigItem::is_null() const
{
    return m_data->get_null();
}



template <typename T, typename Enable = void>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T> > : std::true_type {};



template <class T>
T ConfigItem::get() const
{
    ASSERT(!is_optional<T>() || (m_def && m_def->belongs_to.empty()) || (!m_def && m_is_nullable));
    if constexpr (is_optional<T>())
        if (m_data->get_null())
            return std::nullopt;

    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, std::optional<bool>>) {
        ASSERT(m_type == ConfigItemType::Bool);
        return static_cast<ConfigItemValueBool*>(m_data)->get();
    }
    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, std::optional<int>>) {
        ASSERT(m_type == ConfigItemType::Int);
        return static_cast<ConfigItemValueInt*>(m_data)->get();
    }
    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, std::optional<double>>) {
        if (m_type == ConfigItemType::Double)
            return static_cast<ConfigItemValueDouble*>(m_data)->get();
        else if (m_type == ConfigItemType::FloatOrPercent) {
            ASSERT(!static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent());
            return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->get();
        }
        else
            PANIC();
    }
    if constexpr (std::is_same_v<T, Percentage> || std::is_same_v<T, std::optional<Percentage>>) {
        if (m_type == ConfigItemType::Percent)
            return static_cast<ConfigItemValuePercent*>(m_data)->get();
        else if (m_type == ConfigItemType::FloatOrPercent) {
            ASSERT(static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent());
            return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->get();
        }
        else
            PANIC();
    }
    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::optional<std::string>>) {
        ASSERT(m_type == ConfigItemType::String);
        return static_cast<ConfigItemValueString*>(m_data)->get();
    }
    PANIC();
    throw std::exception(); // to silence a warning
}



template<class T>
void ConfigItem::set(T value)
{
    if constexpr (std::is_same_v<T, bool>) {
        ASSERT(m_type == ConfigItemType::Bool);
        static_cast<ConfigItemValueBool*>(m_data)->set(value);
    }
    else if constexpr (std::is_same_v<T, int>) {
        ASSERT(m_type == ConfigItemType::Int);
        static_cast<ConfigItemValueInt*>(m_data)->set(value);
    }
    else if constexpr (std::is_same_v<T, double>) {
        if (m_type == ConfigItemType::Double)
            static_cast<ConfigItemValueDouble*>(m_data)->set(value);
        else if (m_type == ConfigItemType::FloatOrPercent) {
            static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set(value);
            static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set_percent(false);
        }
        else
            PANIC();
    }
    else if constexpr (std::is_same_v<T, Percentage>) {
        if (m_type == ConfigItemType::Percent)
            static_cast<ConfigItemValuePercent*>(m_data)->set(double(value));
        else if (m_type == ConfigItemType::FloatOrPercent) {
            static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set(double(value));
            static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set_percent(true);
        } else {
            PANIC();
        }
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        ASSERT(m_type == ConfigItemType::String);
        static_cast<ConfigItemValueString*>(m_data)->set(value);
    }
    else if constexpr (std::is_same_v<T, const char*>) {
        this->set(std::string(value));
    }
    else {
        PANIC();
        throw std::exception(); // silence warning
    }
}



bool ConfigItem::is_percent() const {
    ASSERT(m_type == ConfigItemType::FloatOrPercent);
    return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent();
}



void ConfigItem::set_enum_from_string(std::string_view value) {
    ASSERT(m_type == ConfigItemType::Enum);
    for (const EnumValueDef& evd : def().enum_values) {
        if (evd.str_serialized == value) {
            static_cast<ConfigItemValueEnum*>(m_data)->set(int(evd.enum_value));
            return;
        }
    }
    PANIC();
}

std::pair<std::string_view, std::string_view> ConfigItem::get_enum_strings() const {
    for (const EnumValueDef& evd : def().enum_values)
        if (evd.enum_value == static_cast<ConfigItemValueEnum*>(m_data)->get())
            return std::make_pair(std::string_view(evd.str_serialized), std::string_view(evd.str_ui));
    PANIC();
    throw std::exception();
}

void ConfigItem::set_enum_from_int(int value)
{
    ASSERT(std::find_if(def().enum_values.begin(), def().enum_values.end(),
        [value](const EnumValueDef& evd) { return evd.enum_value == value; }) != def().enum_values.end());
    static_cast<ConfigItemValueInt*>(m_data)->set(int(value));
}

int ConfigItem::get_enum_as_int() const
{
    return static_cast<int>(static_cast<ConfigItemValueInt*>(m_data)->get());
}



template<class T>
std::vector<T>& ConfigItem::vec()
{
    if constexpr (std::is_same_v<T, bool>) {
        ASSERT(m_type == ConfigItemType::Bools);
        return static_cast<ConfigItemValueBools*>(m_data)->get();
    }
    else if constexpr (std::is_same_v<T, int>) {
        ASSERT(m_type == ConfigItemType::Ints);
        return static_cast<ConfigItemValueInts*>(m_data)->get();
    }
    else if constexpr (std::is_same_v<T, double>) {
        ASSERT(m_type == ConfigItemType::Doubles);
        return static_cast<ConfigItemValueDoubles*>(m_data)->get();
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        ASSERT(m_type == ConfigItemType::Strings);
        return static_cast<ConfigItemValueStrings*>(m_data)->get();
    }
    PANIC();
    throw std::exception(); // silence warning
}



ConfigItem& ConfigBox::opt(const std::string_view key) {
    auto it = std::lower_bound(m_items.begin(), m_items.end(), key,
        [](const ConfigItem& i, const auto& val) { return i.name() < val; });
    if (it != m_items.end() && it->name() == key)
        return *it;
    PANIC();
    throw std::exception(); // to silence a warning
}



std::optional<const ConfigItem*> ConfigBox::has(const std::string_view key) const {
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



ConfigBox::ConfigBox(const ConfigDefinitions& defs, std::string_view type)
: m_type{type}
{
    for (const ConfigItemDef& def : defs.defs()) {
        bool belongs_here = false;
        for (const auto& list : { &def.belongs_to, &def.belongs_to_optional })
            belongs_here |= std::any_of(list->begin(), list->end(),
                [&type](const auto& t) { return type == t; });
        if (belongs_here)
            m_items.emplace_back(ConfigItem(def, type));
    }
}



const ConfigItem& ConfigView::opt(const std::string_view key, int extruder_idx) const
{
    for (auto rev_it = m_config_boxes.rbegin(); rev_it != m_config_boxes.rend(); ++rev_it) {
        if (auto opt = rev_it->get().has(key))
            return **opt;
    }
    return m_full_config.opt(key, extruder_idx);
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



void FullConfig::add(const std::vector<std::reference_wrapper<const ConfigBox>>& boxes)
{
    std::set<std::string> box_types;

    for (size_t box_id=0; box_id<boxes.size(); ++box_id) {
        const ConfigBox& box = boxes[box_id];
        box_types.insert(std::string(box.type()));

        for (const ConfigItem& item : box) {
            if (auto it_s = m_single_items.find(item.name()); it_s != m_single_items.end()) {
                ASSERT(m_multi_items.find(item.name()) == m_multi_items.end());
                m_multi_items.emplace(item.name(), std::vector<ConfigItem>(boxes.size(), it_s->second));
                m_single_items.erase(it_s);
            }

            auto it_m = m_multi_items.find(item.name());
            if (it_m == m_multi_items.end())
                it_m = m_multi_items.emplace(item.name(), std::vector<ConfigItem>()).first;

            if (box_id < it_m->second.size() && !item.is_null()) {
                // The element is already there, we should override it.
                it_m->second[box_id] = item;
            }
            if (box_id >= it_m->second.size()) {
                // Element is not there. Insert it even if it is null.
                it_m->second.emplace_back(item);
            }
        }
    }
    ASSERT(box_types.size() == 1);
}



std::vector<std::string> FullConfig::diff_keys(const FullConfig& other) const
{
    return ConfigView(*this).diff_keys(ConfigView(other));
}



std::vector<std::string> ConfigView::diff_keys(const ConfigView& other) const
{
    // Reminder: FullConfig is our friend class.
    ASSERT(this->m_full_config.name() == other.m_full_config.name());
    std::vector<std::string> out;

    for (const auto& [key, item] : this->m_full_config.m_single_items) {
        auto it = other.m_full_config.m_single_items.find(key);
        if (it == other.m_full_config.m_single_items.end() || it->second != item)
            out.emplace_back(key);
    }
    for (const auto& [key, items] : this->m_full_config.m_multi_items) {
        auto it = other.m_full_config.m_multi_items.find(key);
        if (it == other.m_full_config.m_multi_items.end() || items != it->second) {
            out.emplace_back(key);
            break;
        }
    }
    // Now the extra boxes:
    ASSERT(this->m_config_boxes.size() == other.m_config_boxes.size());
    for (size_t i = 0; i < m_config_boxes.size(); ++i) {
        std::vector<std::string> diff = m_config_boxes[i].get().diff_keys(other.m_config_boxes[i].get());
        out.insert(out.end(), diff.begin(), diff.end());
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}



const ConfigItem& FullConfig::opt(const std::string_view key, int extruder_idx) const {
    if (extruder_idx == -1) {
        auto it = m_single_items.find(std::string(key));
        ASSERT(it != m_single_items.end());
        return it->second;
    } else {
        auto it = m_multi_items.find(std::string(key));
        ASSERT(it != m_multi_items.end());
        ASSERT(extruder_idx < it->second.size());
        return it->second[extruder_idx];
    }
}



// Explicit instantiations for getters.
template bool ConfigItem::get<bool>() const;
template int ConfigItem::get<int>() const;
template double ConfigItem::get<double>() const;
template Percentage ConfigItem::get<Percentage>() const;
template std::string ConfigItem::get<std::string>() const;
template std::optional<bool> ConfigItem::get<std::optional<bool>>() const;
template std::optional<int> ConfigItem::get<std::optional<int>>() const;
template std::optional<double> ConfigItem::get<std::optional<double>>() const;
template std::optional<Percentage> ConfigItem::get<std::optional<Percentage>>() const;
template std::optional<std::string> ConfigItem::get<std::optional<std::string>>() const;

// Explicit instantiations for setters.
template void ConfigItem::set(bool);
template void ConfigItem::set(int);
template void ConfigItem::set(double);
template void ConfigItem::set(Percentage);
template void ConfigItem::set(std::string);
template void ConfigItem::set(const char*);

// And the respective explicit instantiations.
template std::vector<bool>& ConfigItem::vec();
template std::vector<int>& ConfigItem::vec();
template std::vector<double>& ConfigItem::vec();
template std::vector<std::string>& ConfigItem::vec();

} // namespace Slic3r::Domain
