#include "Slic3r/App/Platform/KeyboardShortcut.hpp"

namespace Slic3r::App::Platform {

std::string KeyboardShortcut::to_string() const
{
    std::string shortcut;
    if (std::string str_modifiers = Platform::to_string(modifiers);
        !str_modifiers.empty())
    {
        shortcut += str_modifiers;
    }
    shortcut += Platform::to_string(key);

    return shortcut;
}
bool KeyboardShortcut::has_modifier() const
{
    return modifiers != 0;
}

} // Slic3r::App::Platform
