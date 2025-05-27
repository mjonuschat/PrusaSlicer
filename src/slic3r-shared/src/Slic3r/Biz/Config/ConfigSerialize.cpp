#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"

#include "Slic3r/Domain/Types.hpp"
#include "boost/algorithm/string.hpp"

#include <set>

namespace Slic3r::Biz {

using Domain::ConfigItem;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::EnumWrapper;
using Domain::EnumVectorWrapper;
using Domain::Vec2d;

static nlohmann::json serialize_float_or_percent(const FloatOrPercentage& fop)
{
    nlohmann::json j;
    j["value"] = fop.is_percentage() ? fop.percentage().value : fop.float_value();
    j["is_percent"] = fop.is_percentage();
    return j;
}

static nlohmann::json serialize_percent(const Percentage& percentage)
{
    nlohmann::json j;
    j["value"] = percentage.value;
    j["is_percent"] = true;
    return j;
}

static nlohmann::json serialize_point(const Vec2d& p)
{
    return nlohmann::json(std::vector<double>{p.x(), p.y()});
}

static nlohmann::json serialize_points(const std::vector<Vec2d>& pts)
{
    std::vector<std::vector<double>> out;
    for (const Domain::Vec2d& pt : pts)
        out.emplace_back(std::vector<double>{pt.x(), pt.y()});
    return nlohmann::json(out);
}

static nlohmann::json serialize_enums(const std::vector<std::string_view>& strings)
{
    std::vector<std::string> out;
    out.insert(out.end(), strings.begin(), strings.end());
    return nlohmann::json(out);
}

static void serialize_and_append(const ConfigItem& item, nlohmann::json& j)
{
    j[item.name()] = nullptr;
    auto& jval = j.back();
    if (item.is_null())
        return;

    item.visit([&](auto&& item_value){
        using ValueType = std::remove_cvref_t<decltype(item_value)>;
        if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            jval = item_value.get_string();
        } else if constexpr (std::is_same_v<ValueType, EnumVectorWrapper>) {
            jval = serialize_enums(item_value.get_strings());
        } else if constexpr (std::is_same_v<ValueType, Vec2d>) {
            jval = serialize_point(item_value);
        } else if constexpr (std::is_same_v<ValueType, std::vector<Vec2d>>) {
            jval = serialize_points(item_value);
        } else if constexpr (
            std::is_same_v<ValueType, Percentage>
            ||std::is_same_v<ValueType, FloatOrPercentage>
        ) {
            jval = serialize_float_or_percent(item_value);
        } else if constexpr (std::is_same_v<ValueType, std::optional<int>>) {
            if (item_value)
                jval = *item_value;
            else
                jval = nullptr;
        } else if constexpr (
            std::is_same_v<ValueType, int>
            || std::is_same_v<ValueType, double>
            || std::is_same_v<ValueType, std::string>
            || std::is_same_v<ValueType, std::vector<int>>
            || std::is_same_v<ValueType, std::vector<double>>
            || std::is_same_v<ValueType, std::vector<std::string>>
        ) {
            jval = item.get<ValueType>();
        } else {
            PANIC("Cannot serialize value!");
        }
    });
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
