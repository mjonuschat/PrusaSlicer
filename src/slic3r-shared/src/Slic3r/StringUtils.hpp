#pragma once

#include <string>

namespace Slic3r {

/**
 * List of illegal characters
 */
static constexpr char illegal_characters[] = "<>:/\\|?*\"";

/**
 * Function to detect containing of the illegal characters
 */
inline bool has_illegal_characters(const std::string& str)
{
	for (size_t i = 0; i < std::strlen(illegal_characters); i++)
		if (str.find_first_of(illegal_characters[i]) != std::string::npos)
			return true;

	return false;
}

}