#pragma once
#include <cstdint>
#include <type_traits>

#include "KeyModifers.hpp"

namespace Slic3r::App::Platform {

#ifdef None
// There is a special place in hell for people who create common word defines
// without prefixes like the Xlib devs
#undef None
#endif

enum class MouseButton : uint8_t {
    NoButton = 0,
    Left = 1,
    Middle = 1 << 1,
    Right = 1 << 2,
};

using MouseButtons = std::underlying_type_t<MouseButton>;

class MouseEvent {
public:
    enum class Type : uint8_t {
        Move = 0,
        ButtonDown,
        ButtonUp,
        Wheel,
        Enter,
        Leave
    };

    MouseEvent(Type type, MouseButton button, int x, int y, float wheel_delta_x, float wheel_delta_y, KeyModifiers key_modifiers)
        : m_type(type), m_button(button), m_x(x), m_y(y), m_wheel_delta_x(wheel_delta_x), m_wheel_delta_y(wheel_delta_y), m_key_modifiers(key_modifiers)
    {}

    [[nodiscard]] Type get_type() const { return m_type; }
    [[nodiscard]] MouseButton get_button() const { return m_button; }
    [[nodiscard]] int get_x() const { return m_x; }
    [[nodiscard]] int get_y() const { return m_y; }
    [[nodiscard]] float get_wheel_delta_x() const { return m_wheel_delta_x; }
    [[nodiscard]] float get_wheel_delta_y() const { return m_wheel_delta_y; }
    [[nodiscard]] KeyModifiers get_key_modifiers() const { return m_key_modifiers; }
private:
    Type m_type;
    MouseButton m_button;
    int m_x{0};
    int m_y{0};
    float m_wheel_delta_x{0};
    float m_wheel_delta_y{0};
    KeyModifiers m_key_modifiers;
};

}
