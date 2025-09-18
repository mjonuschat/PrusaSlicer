#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace  Slic3r::Domain::Preset {

std::string derive_name(const std::string& name, const std::string& parent_name)
{

    const size_t parent_separator_pos = parent_name.find('@');

    if (parent_separator_pos == std::string::npos)
        return name.empty() ? parent_name : name;

    return name + parent_name.substr(parent_separator_pos);
}

bool is_public_name(const std::string& name)
{
    return !name.empty() && name[0] != '*';
}

}

