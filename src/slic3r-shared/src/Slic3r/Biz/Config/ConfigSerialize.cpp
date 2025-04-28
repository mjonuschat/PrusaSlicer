#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"

#include "boost/algorithm/string.hpp"

#include <set>

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

static nlohmann::json serialize_point(const ConfigItem& item)
{
    ASSERT(item.type() == ConfigItemType::Point);
    const auto& p = item.get<Domain::Vec2d>();
    return nlohmann::json(std::vector<double>{{p.x()}, { p.y() }});
}

static nlohmann::json serialize_points(const ConfigItem& item)
{
    ASSERT(item.type() == ConfigItemType::Points);
    const auto& pts = item.get<std::vector<Domain::Vec2d>>();
    std::vector<std::vector<double>> out;
    for (const Domain::Vec2d& pt : pts)
        out.emplace_back(std::vector<double>{pt.x(), pt.y()});
    return nlohmann::json(out);
}

static nlohmann::json serialize_enums(const ConfigItem& item)
{
    ASSERT(item.type() == ConfigItemType::Enums);
    const auto& strings = item.get_enums_strings();
    std::vector<std::string> out;
    for (const auto& [serialized, ui] : strings)
        out.emplace_back(serialized);
    return nlohmann::json(out);
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
        case ConfigItemType::Point : jval = serialize_point(item); break;
        
        case ConfigItemType::Bools   : jval = item.vec<bool>(); break;
        case ConfigItemType::Ints    : jval = item.vec<int>(); break;
        case ConfigItemType::Doubles : jval = item.vec<double>(); break;
        case ConfigItemType::Strings : jval = item.vec<std::string>(); break;
        case ConfigItemType::Points  : jval = serialize_points(item); break;
        case ConfigItemType::Enums   : jval = serialize_enums(item); break;

        case ConfigItemType::IntOptional :
            if (const auto& opt_int = item.get<std::optional<int>>(); opt_int)
                jval = *opt_int;
            else
                jval = nullptr;
            break;

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



nlohmann::json serialize(const Domain::ConfigBox& box, bool omit_null_overrides /*true*/)
{
    nlohmann::json out;
    for (const ConfigItem& item : box) {
        if (omit_null_overrides && item.is_null() && item.def().location != box.type())
            continue;
        serialize_and_append(item, out);
    }
    return out;
}



nlohmann::json serialize_as_vector(const std::vector<std::reference_wrapper<const Domain::ConfigBox>> boxes)
{
    ASSERT(! boxes.empty());
    ASSERT(std::all_of(boxes.begin(), boxes.end(), [&boxes](const auto& box_ref) {
        return box_ref.get().type() == boxes.front().get().type();
    }));

    // Create a JSON object from each box individually.
    std::vector<nlohmann::json> json_objects;
    for (const auto& box : boxes)
        json_objects.emplace_back(serialize(box.get(), false));

    // Vectorization of the individual json objects. Assumes that the keys are the same.
    nlohmann::json combined_json = nlohmann::json::object();
    for (auto it = json_objects[0].items().begin(); it != json_objects[0].items().end(); ++it) {
        const std::string& current_key = it.key();
        nlohmann::json value_array = nlohmann::json::array();
        for (const auto& obj : json_objects)
            value_array.push_back(obj[current_key]);
        combined_json[current_key] = std::move(value_array);
    }

    // Remove all vectors which are full of nulls - but only for overrides.
    for (auto it = combined_json.begin(); it != combined_json.end(); ) {
        if (boxes.front().get().opt(it.key()).def().location == boxes.front().get().type()) {
            ++it;
            continue; // Not an override, apparently an optional value.
        }
        ASSERT(it.value().is_array());
        if (const auto& arr = it.value();
            ! arr.empty() && std::all_of(arr.begin(), arr.end(), [](const auto& e) { return e.is_null(); }))
            it = combined_json.erase(it);
        else
            ++it;
    }

    return combined_json;
}



std::string serialize(
    std::vector<
        std::variant<
            std::reference_wrapper<const Domain::ConfigBox>,
            std::vector<std::reference_wrapper<const Domain::ConfigBox>>
        >
    > input,
    int indent,
    bool prepend_semicolons)
{
    std::set<std::string> box_names;

    nlohmann::ordered_json complete_json;
    for (const auto& var : input) {
        if (std::holds_alternative<std::reference_wrapper<const Domain::ConfigBox>>(var)) {
            const auto& box = std::get<std::reference_wrapper<const Domain::ConfigBox>>(var).get();
            box_names.emplace(box.type());
            complete_json[box.type()] = serialize(box);
        } else {
            const auto& boxes = std::get<std::vector<std::reference_wrapper<const Domain::ConfigBox>>>(var);
            box_names.emplace(boxes.front().get().type());
            complete_json[boxes.front().get().type()] = serialize_as_vector(boxes);
        }
    }
    std::string str = complete_json.dump(indent);

    // Now a little minification to make the result a bit more readable.
    // Only removes newlines and leading spaces, so the meaning stays the same.
    auto count_indent = [](const std::string& s) -> int { int i=0; while (s[i] == ' ') ++i; return i; };
    std::vector<std::string> lines;
    boost::split(lines, str, boost::is_any_of("\n"));
    for (auto& line : lines)
        line += '\n';
    std::string tmp;
    for (const std::string& box_name : box_names) {
        std::string line_start = std::string("\"") + box_name;
        for (size_t line_id=0; line_id<lines.size(); ++line_id) {
            if (lines[line_id].find(line_start) == indent) {
                int box_indent = count_indent(lines[line_id]);
                size_t i = line_id + 1;
                while (true) {
                    int ind = count_indent(lines[i]);
                    if (ind <= box_indent)
                        break;
                    if (ind > box_indent + indent || (ind == box_indent + indent && lines[i][ind] != '\"')) {
                        boost::trim_left(lines[i]);
                        ASSERT(lines[i-1].back() == '\n');
                        lines[i-1].pop_back();
                    }
                    ++i;
                }
            }
        }
    }
    str = {};
    for (const std::string& line : lines) {
        if (prepend_semicolons)
            str += "; ";
        str += line;
    }

    return str;
}

} // namespace Slic3r::Biz
