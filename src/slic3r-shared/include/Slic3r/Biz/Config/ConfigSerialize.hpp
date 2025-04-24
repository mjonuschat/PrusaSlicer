#pragma once

#include <string>
#include <variant>

#include "nlohmann/json.hpp"

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Biz {

// Returns serialized value (or values) of the config option.
std::variant<std::string, std::vector<std::string>> serialize_to_string(const Domain::ConfigItem& item);

// Serializes a given ConfigBox.
nlohmann::json serialize(const Domain::ConfigBox& box, bool omit_null_overrides = true);

// Given list of boxes of the same type, serializes the content such that each key
// appears once and items from individual boxes end up as vector elements.
// Vector which belong to overrides and which are full of nulls are omitted.
nlohmann::json serialize_as_vector(const std::vector<std::reference_wrapper<const Domain::ConfigBox>> boxes);

// Given list of ConfigBoxes and vectors of ConfigBoxes, serializes all of that into a single string.
std::string serialize(
	std::vector<
		std::variant<
		    std::reference_wrapper<const Domain::ConfigBox>,
	        std::vector<std::reference_wrapper<const Domain::ConfigBox>>
	    >
	> input,
	int indent = 4);

} // namespace Slic3r::Biz
