#pragma once

#include <cstdint>
#include "KeyCode.hpp"
#include "KeyModifers.hpp"

namespace Slic3r::App::Platform {

class KeyboardEvent
{
public:
    enum class Type : uint8_t {
        KeyDown = 0,
        KeyUp = 1,
        COUNT = KeyUp
    };

    KeyboardEvent(Type type, KeyCode code, KeyModifiers modifiers)
        : m_type(type), m_code(code), m_key_modifiers(modifiers)
    {}

    Type get_type() const { return m_type; }
    KeyCode get_code() const { return m_code; }
    KeyModifiers get_key_modifiers() const { return m_key_modifiers; }

private:
    Type m_type;
    KeyModifiers m_key_modifiers;
    KeyCode m_code;
};

}
