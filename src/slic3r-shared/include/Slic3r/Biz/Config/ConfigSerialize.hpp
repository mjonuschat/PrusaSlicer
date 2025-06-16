#pragma once

#include <string>
#include <variant>

#include "nlohmann/json_fwd.hpp"

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Domain {
void to_json(nlohmann::ordered_json& json_value, const Domain::BoxOrBoxesVector& box_or_boxes_vector);
}

namespace Slic3r::Biz {

// Returns serialized value (or values) of the config option.
std::variant<std::string, std::vector<std::string>> serialize_to_string(const Domain::ConfigItem& item);

// Given list of ConfigBoxes and vectors of ConfigBoxes, serializes all of that into a single string.
std::string serialize(const Domain::BoxOrBoxesVector& input, int indent, bool prepend_semicolons);

} // namespace Slic3r::Biz
