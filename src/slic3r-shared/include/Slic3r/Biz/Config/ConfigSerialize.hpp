#pragma once

#include <string>
#include <variant>

#include "nlohmann/json.hpp"

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Biz {

nlohmann::json serialize(const Domain::ConfigBox& box);

// Returns serialized value (or values) of the config option.
std::variant<std::string, std::vector<std::string>> serialize_to_string(const Domain::ConfigItem& item);

} // namespace Slic3r::Biz
