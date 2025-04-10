#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"

#include "boost/algorithm/string.hpp"


static void serialize_and_append(const ConfigItem& item, nlohmann::json& j)
{
	j[item.name()] = nullptr;
	auto& jval = j.back();
	if (item.is_null())
		return;

	switch (item.def().type) {
		case ConfigItemType::Bool   : jval = item.get_bool(); break;
		case ConfigItemType::Int    : jval = item.get_int(); break;
	    case ConfigItemType::Double : jval = item.get_double(); break;
		case ConfigItemType::String : jval = item.get_str(); break;
		case ConfigItemType::Enum   : jval = item.get_enum_strings().first; break;
		
		case ConfigItemType::Bools   : jval = item.bools(); break;
		case ConfigItemType::Ints    : jval = item.ints(); break;
	    case ConfigItemType::Doubles : jval = item.doubles(); break;
		case ConfigItemType::Strings : jval = item.strings(); break;
		default : PANIC();
	}
	return;
}


static std::string trim_quotes(const std::string& json_str)
{
	std::string out(json_str);
	boost::trim_if(out, boost::is_any_of("\""));
	return out;
}


std::variant<std::string, std::vector<std::string>> serialize_to_string(const ConfigItem& item)
{
	nlohmann::json j;
	serialize_and_append(item, j);

	ASSERT(! j.empty());

    auto it = j.begin(); // Get the first (and assumed to be only) key-value pair
    const auto& value = it.value();

	if (value.is_array()) {
        std::vector<std::string> serialized_elements;
        for (const auto& element : value) {
            serialized_elements.push_back(trim_quotes(element.dump(-1, ' ', false)));
        }
        return serialized_elements;
    } else
        return trim_quotes(value.dump(-1, ' ', false));
}



nlohmann::json serialize(const ConfigBox& box)
{
	nlohmann::json out;
	for (const ConfigItem& item : box) {
		if (item.is_null() && std::find(item.def().belongs_to.begin(),
			item.def().belongs_to.end(),
			std::string(box.type()))
			== item.def().belongs_to.end()) {
			// Null items are only serialized if they are mandatory for the box type.
			continue;
		}
		serialize_and_append(item, out);
	}
	return out;
}
