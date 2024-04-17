#pragma once

#include <cstdint>
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

}