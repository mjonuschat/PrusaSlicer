#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"

#include "boost/algorithm/string.hpp"

namespace Slic3r::Biz {

using ConfigItem = Domain::ConfigItem;
using ConfigItemType = Domain::ConfigItemType;
using FloatOrPercentage = Domain::FloatOrPercentage;
using Percentage = Domain::Percentage;

static nlohmann::json serialize_float_or_percent(const ConfigItem& item)
{
    nlohmann::json j;
    if (item.type() == ConfigItemType::Percent) {
        j["value"] = item.get<Percentage>().value;
        j["is_percent"] = true;
    } else if (item.type() == ConfigItemType::FloatOrPercent) {
        FloatOrPercentage fop = item.get<FloatOrPercentage>();
        j["value"] = fop.is_percentage() ? fop.percentage().value : fop.float_value();
        j["is_percent"] = fop.is_percentage();
    } else
        PANIC("This function cannot be used with this ConfigItemType.");
    return j;
}

static void serialize_and_append(const ConfigItem& item, nlohmann::json& j)
{
    j[item.name()] = nullptr;
    auto& jval = j.back();
    if (item.is_null())
        return;

    switch (item.def().type) {
        case ConfigItemType::Bool   : jval = item.get<bool>(); break;
        case ConfigItemType::Int    : jval = item.get<int>(); break;
        case ConfigItemType::Double : jval = item.get<double>(); break;
        case ConfigItemType::String : jval = item.get<std::string>(); break;
        case ConfigItemType::Enum   : jval = item.get_enum_strings().first; break;
        case ConfigItemType::Percent : [[fallthrough]];
        case ConfigItemType::FloatOrPercent : jval = serialize_float_or_percent(item); break;
        
        case ConfigItemType::Bools   : jval = item.vec<bool>(); break;
        case ConfigItemType::Ints    : jval = item.vec<int>(); break;
        case ConfigItemType::Doubles : jval = item.vec<double>(); break;
        case ConfigItemType::Strings : jval = item.vec<std::string>(); break;
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



nlohmann::json serialize(const Domain::ConfigBox& box)
{
    nlohmann::json out;
    for (const ConfigItem& item : box) {
        if (item.is_null() && item.def().location != box.type()) {
            // Null items are only serialized if they are mandatory for the box type.
            continue;
        }
        serialize_and_append(item, out);
    }
    return out;
}

} // namespace Slic3r::Biz
