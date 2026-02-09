#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace  Slic3r::Domain::Preset {

std::optional<std::string_view> PresetNode::short_name() const
{
    if (!name.has_value())
        return std::nullopt;
    return Domain::Preset::short_name(name.value());
}

std::string derive_name(const std::string& name, const std::string& parent_name)
{

    const size_t parent_separator_pos = parent_name.find('@');

    if (parent_separator_pos == std::string::npos)
        return name.empty() ? parent_name : name;

    return name + parent_name.substr(parent_separator_pos);
}

std::string_view short_name(const std::string& name)
{
    size_t idx = name.find('@');
    if (idx == 0 || idx == std::string_view::npos)
        return name;
    while (idx > 0 && name[idx] == ' ') idx--;
    return std::string_view{name.data(), idx};
}

bool is_public_name(const std::string& name)
{
    return !name.empty() && name[0] != '*';
}

}

