#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

namespace Slic3r::App::Platform {

enum class KeyModifier : uint8_t
{
    None = 0,
    Alt = 1 << 0,
    Shift = 1 << 1,
    Meta = 1 << 2,
    Ctrl = 1 << 3,
};

typedef std::underlying_type_t<KeyModifier> KeyModifiers;

inline std::string to_string(KeyModifier modifier)
{
    switch (modifier) {
    case KeyModifier::Alt:
        return "Alt";
    case KeyModifier::Shift:
        return "Shift";
    case KeyModifier::Ctrl:
        return "Ctrl"; // need to respect Platform ("Cmd" for OSX)
    default:
        return std::string();
    }
}

inline std::string to_string(KeyModifiers modifiers)
{
    bool shift_down = (modifiers & KeyModifiers(KeyModifier::Shift)) != 0;
    bool ctrl_down  = (modifiers & KeyModifiers(KeyModifier::Ctrl)) != 0;
    bool alt_down   = (modifiers & KeyModifiers(KeyModifier::Alt)) != 0;

    std::string ret;
    if (shift_down) {
        ret += to_string(KeyModifier::Shift) + "+";
    }
    if (ctrl_down) {
        ret += to_string(KeyModifier::Ctrl) + "+";
    }
    if (alt_down) {
        ret += to_string(KeyModifier::Alt) + "+";
    }

    return ret;
}


}