#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"

#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Config/ConfigJson.hpp" // IWYU pragma: keep
#include "boost/algorithm/string.hpp"

#include <set>

static std::vector<std::string> to_strings(const std::vector<std::string_view>& strings)
{
    std::vector<std::string> out;
    out.insert(out.end(), strings.begin(), strings.end());
    return out;
}

namespace Slic3r::Domain {
[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const ConfigItem& item) {
    item.visit([&](auto&& item_value) {
        using ValueType = std::remove_cvref_t<decltype(item_value)>;
        if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            json_value = item_value.get_string();
        } else if constexpr (std::is_same_v<ValueType, EnumVectorWrapper>) {
            json_value = to_strings(item_value.get_strings());
        } else {
            json_value = item.get<ValueType>();
        }
    });
}

[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const Domain::ConfigBox& box)
{
    for (const ConfigItem& item : box.overrides.all_items()) {
        if (box.overrides.get(item.name())) {
            json_value[item.name()] = item;
        } else {
            json_value[item.name()] = nullptr;
        }
    }

    for (const ConfigItem& item : box.items) {
        json_value[item.name()] = item;
    }
}

// Given list of boxes of the same type, serializes the content such that each key
// appears once and items from individual boxes end up as vector elements.
// Vector which belong to overrides and which are full of nulls are omitted.
[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const BoxRefs& boxes)
{
    ASSERT(!boxes.empty());
    ASSERT(std::all_of(boxes.begin(), boxes.end(), [&boxes](const auto& box_ref) {
        return box_ref.get().location == boxes.front().get().location;
    }));

    // Create a JSON object from each box individually.
    std::vector<nlohmann::ordered_json> json_objects;
    for (const auto& box : boxes)
        json_objects.emplace_back(box.get());

    // Vectorization of the individual json objects. Assumes that the keys are the same.
    json_value = nlohmann::ordered_json::object();
    for (auto it = json_objects[0].items().begin(); it != json_objects[0].items().end(); ++it) {
        const std::string& current_key = it.key();
        nlohmann::ordered_json value_array = nlohmann::ordered_json::array();
        for (const auto& obj : json_objects)
            value_array.push_back(obj[current_key]);
        json_value[current_key] = std::move(value_array);
    }

    // Remove all vectors which are full of nulls - but only for overrides.
    for (auto it = json_value.begin(); it != json_value.end(); ) {
        if (!boxes.front().get().overrides.contains(it.key())) {
            ++it;
            continue; // Not an override, apparently an optional value.
        }
        ASSERT(it.value().is_array());
        if (const auto& arr = it.value();
            ! arr.empty() && std::all_of(arr.begin(), arr.end(), [](const auto& e) { return e.is_null(); }))
            it = json_value.erase(it);
        else
            ++it;
    }
}

void to_json(
    nlohmann::ordered_json& json_value, const BoxOrBoxesVector& box_or_boxes_vector
)
{
    for (const auto& box_or_boxes : box_or_boxes_vector) {
        std::visit([&](auto&& box_or_boxes){
            using ValueType = std::remove_cvref_t<decltype(box_or_boxes)>;
            std::string location_name;
            if constexpr(std::is_same_v<ValueType, BoxRef>) {
                location_name = get_location_name(box_or_boxes.get().location);
            } else {
                location_name = get_location_name(box_or_boxes.front().get().location);
            }
            json_value[location_name] = box_or_boxes;
        }, box_or_boxes);
    }
}
}

namespace Slic3r::Biz {

using Domain::ConfigItem;
using Domain::Vec2d;
using Domain::BoxOrBoxesVector;
using Domain::BoxRefs;
using Domain::BoxRef;
using Domain::ConfigLocation;

static std::string trim_quotes(const std::string& json_str)
{
    std::string out(json_str);
    boost::trim_if(out, boost::is_any_of("\""));
    return out;
}


std::variant<std::string, std::vector<std::string>> serialize_to_string(const ConfigItem& item)
{
    nlohmann::ordered_json j;
    j[item.name()] = item;

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



std::string beautify_json(const Domain::BoxOrBoxesVector& box_or_boxes_vector, int indent)
{
    return beautify_json(nlohmann::ordered_json(box_or_boxes_vector), indent);
}



std::string beautify_json(
    const nlohmann::ordered_json& complete_json,
    int indent)
{
    std::string str = complete_json.dump(indent);

    std::set<std::string> box_names;
    for (const auto& [key, _] : complete_json.items()) {
        box_names.insert(key);
    }

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

    ASSERT(!lines.empty());
    std::vector<std::string> aggregated_lines{lines.front()};
    for (std::size_t i{1}; i < lines.size(); ++i) {
        const std::string& previous_line{lines[i-1]};
        const std::string& line{lines[i]};
        if (previous_line.back() == ',') {
            aggregated_lines.back() += " " + line;
        } else if (previous_line.back() != '\n') {
            aggregated_lines.back() += line;
        } else {
            aggregated_lines.push_back(line);
        }
    }

    str = {};
    for (const std::string& line : aggregated_lines) {
        str += line;
    }

    return str;
}

} // namespace Slic3r::Biz
