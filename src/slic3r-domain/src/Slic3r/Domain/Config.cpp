#include "Slic3r/Domain/Config.hpp"

#include "ConfigItemValue.hpp"

#include <algorithm>


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



void ConfigItem::set_null(bool null)
{
    ASSERT(m_is_nullable);
    m_data->set_null(null);
}

bool ConfigItem::is_null() const
{
    return m_data->get_null();
}



void ConfigItem::set_bool(bool value)
{
    ASSERT(m_type == ConfigItemType::Bool);
    static_cast<ConfigItemValueBool*>(m_data)->set(value);
}

bool ConfigItem::get_bool() const
{
    ASSERT(m_type == ConfigItemType::Bool);
    return static_cast<ConfigItemValueBool*>(m_data)->get();
}



void ConfigItem::set_int(int value)
{
    ASSERT(m_type == ConfigItemType::Int);
    static_cast<ConfigItemValueInt*>(m_data)->set(value);
}

int  ConfigItem::get_int() const
{
    ASSERT(m_type == ConfigItemType::Int);
    return static_cast<ConfigItemValueInt*>(m_data)->get();
}



void ConfigItem::set_double(double value)
{
    if (m_type == ConfigItemType::Double)
        static_cast<ConfigItemValueDouble*>(m_data)->set(value);
    else if (m_type == ConfigItemType::FloatOrPercent) {
        static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set(value);
        static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set_percent(false);
    }
	else
		PANIC();
}

double ConfigItem::get_double() const
{
    if (m_type == ConfigItemType::Double)
		return static_cast<ConfigItemValueDouble*>(m_data)->get();
	else if (m_type == ConfigItemType::FloatOrPercent) {
		ASSERT(! static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent());
		return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->get();
    }
	PANIC();
	throw std::exception(); // to silence a warning
}
    
void ConfigItem::set_percent(double value)
{
    ASSERT(m_type == ConfigItemType::FloatOrPercent);
    static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set(value);
	static_cast<ConfigItemValueFloatOrPercent*>(m_data)->set_percent(true);
}

double ConfigItem::get_percent() const
{
    ASSERT(m_type == ConfigItemType::FloatOrPercent);
	ASSERT(static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent());
    return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->get();
}

bool ConfigItem::is_percent() const {
    ASSERT(m_type == ConfigItemType::FloatOrPercent);
    return static_cast<ConfigItemValueFloatOrPercent*>(m_data)->is_percent();
}



void ConfigItem::set_str(const std::string& value)
{
    ASSERT(m_type == ConfigItemType::String);
    static_cast<ConfigItemValueString*>(m_data)->set(value);
}

const std::string& ConfigItem::get_str() const
{
    ASSERT(m_type == ConfigItemType::String);
    return static_cast<ConfigItemValueString*>(m_data)->get();
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



std::vector<bool>& ConfigItem::bools() { ASSERT(m_type == ConfigItemType::Bools); return static_cast<ConfigItemValueBools*>(m_data)->get(); }
const std::vector<bool>& ConfigItem::bools() const { return const_cast<ConfigItem*>(this)->bools(); }

std::vector<int>& ConfigItem::ints() { ASSERT(m_type == ConfigItemType::Ints); return static_cast<ConfigItemValueInts*>(m_data)->get(); }
const std::vector<int>& ConfigItem::ints() const { return const_cast<ConfigItem*>(this)->ints(); }

std::vector<double>& ConfigItem::doubles() { ASSERT(m_type == ConfigItemType::Doubles); return static_cast<ConfigItemValueDoubles*>(m_data)->get(); }
const std::vector<double>& ConfigItem::doubles() const { return const_cast<ConfigItem*>(this)->doubles(); }

std::vector<std::string>& ConfigItem::strings() { ASSERT(m_type == ConfigItemType::Strings); return static_cast<ConfigItemValueStrings*>(m_data)->get(); }
const std::vector<std::string>& ConfigItem::strings() const { return const_cast<ConfigItem*>(this)->strings(); }




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
    ASSERT(std::is_sorted(m_items.begin(), m_items.end()));
}



const ConfigItem& ConfigView::opt(const std::string_view key) const
{
	for (auto rev_it = m_config_boxes.rbegin(); rev_it != m_config_boxes.rend(); ++rev_it) {
		if (auto opt = (*rev_it)->has(key))
			return **opt;
	}
	return m_full_config->opt(key);
}



void FullConfig::add(const ConfigBox* box)
{
    for (const ConfigItem& item : *box) {
		if (auto it_m = m_multi_items.find(item.name()); it_m != m_multi_items.end() && ! item.is_null())
			it_m->second = std::vector<const ConfigItem*>(it_m->second.size(), &item);
        else {
            auto it_s = m_single_items.find(item.name());
            if (it_s == m_single_items.end() || !item.is_null())
                m_single_items[item.name()] = &item;
        }
    }
}



void FullConfig::add(const std::vector<const ConfigBox*> boxes)
{
	std::set<std::string> box_types;

    for (size_t box_id=0; box_id<boxes.size(); ++box_id) {
        const ConfigBox& box = *boxes[box_id];
		box_types.insert(std::string(box.type()));

		for (const ConfigItem& item : box) {
            if (auto it_s = m_single_items.find(item.name()); it_s != m_single_items.end()) {
                ASSERT(m_multi_items.find(item.name()) == m_multi_items.end());
				m_multi_items.emplace(item.name(), std::vector<const ConfigItem*>(box_id, it_s->second));
				m_single_items.erase(it_s);
            }

			auto it_m = m_multi_items.find(item.name());
            if (it_m == m_multi_items.end())
				it_m = m_multi_items.emplace(item.name(), std::vector<const ConfigItem*>()).first;

            if (box_id >= it_m->second.size() || ! item.is_null()) {
				it_m->second.resize(box_id + 1);
				it_m->second[box_id] = &item;
            }
        }
    }
	ASSERT(box_types.size() == 1);
}
