#include "Slic3r/Biz/Config/ConfigJson.hpp"

using nlohmann::ordered_json;

namespace Slic3r::Domain::Advanced {
void from_json(const nlohmann::ordered_json& json_value, Vec2d& vec2d)
{
    json_value.at(0).get_to(vec2d[0]);
    json_value.at(1).get_to(vec2d[1]);
}

void to_json(ordered_json& json_value, const Vec2d& vec2d)
{
    json_value = ordered_json::array({vec2d.x(), vec2d.y()});
}
} // namespace Slic3r::Domain::Advanced

namespace Slic3r::Domain {
void from_json(const ordered_json& json_value, Percentage& percentage)
{
    json_value["value"].get_to(percentage.value);
}

void to_json(ordered_json& json_value, const Percentage& percentage)
{
    json_value = {{"value", percentage.value}, {"is_percent", true}};
}

void from_json(const ordered_json& json_value, FloatOrPercentage& fop)
{
    const auto value{json_value["value"].get<double>()};
    const auto is_percentage{json_value["is_percent"].get<bool>()};

    if (is_percentage) {
        fop = FloatOrPercentage{Percentage{value}};
    } else {
        fop = FloatOrPercentage{value};
    }
}

void to_json(ordered_json& json_value, const FloatOrPercentage& fop)
{
    json_value = {
        {"value", fop.is_percentage() ? fop.percentage().value : fop.float_value()},
        {"is_percent", fop.is_percentage()}
    };
}
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Config {

using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Percentage;
using Slic3r::Domain::FloatOrPercentage;

template<>
bool is_valid<int>(const ordered_json& json_value) {
    return json_value.is_number_integer();
}

template<>
bool is_valid<double>(const ordered_json& json_value) {
    return json_value.is_number();
}

template<>
bool is_valid<std::string>(const ordered_json& json_value) {
    return json_value.is_string();
}

template<>
bool is_valid<bool>(const ordered_json& json_value) {
    return json_value.is_boolean();
}

template<>
bool is_valid<Vec2d>(const ordered_json& json_value)
{
    return json_value.is_array()
        && json_value.size() == 2
        && json_value.at(0).is_number()
        && json_value.at(1).is_number();
}

template<>
bool is_valid<FloatOrPercentage>(const ordered_json& json_value)
{
    return json_value.is_object()
        && json_value.contains("value")
        && json_value.contains("is_percent")
        && json_value["value"].is_number()
        && json_value["is_percent"].is_boolean();
}

template<>
bool is_valid<Percentage>(const ordered_json& json_value)
{
    return is_valid<FloatOrPercentage>(json_value) && json_value["is_percent"].get<bool>();
}

} // namespace Slic3r::Biz::Config
