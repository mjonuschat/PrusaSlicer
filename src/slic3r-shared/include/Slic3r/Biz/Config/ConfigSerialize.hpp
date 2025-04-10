#pragma once

#include <string>
#include <variant>

#include "nlohmann/json.hpp"

#include "Slic3r/Domain/Config.hpp"



nlohmann::json serialize(const ConfigBox& box);
std::variant<std::string, std::vector<std::string>> serialize_to_string(const ConfigItem& item);
