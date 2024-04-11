#pragma once

#include <memory>
#include <vector>

#include "IView.hpp"

namespace Slic3r::App::View {

struct MouseEvent {
    enum class Type {
        MOVE, BUTTON_DOWN, BUTTON_UP, WHEEL
    };

    enum class Button: uint8_t {
        LEFT = 0,
        MIDDLE = 1,
        RIGHT = 2,
        _COUNT = RIGHT
    };

    Type type;
    float position_x;
    float position_y;
    float wheel;
    bool buttons[uint8_t(Button::_COUNT)];
};

struct KeyEvent {

};

class BaseRenderModule
{
public:
    virtual ~BaseRenderModule() = default;

    virtual void activated() {}
    virtual void deactivated() {}

    virtual void render_imgui();
    virtual void render_scene() {}
    virtual void on_mouse_event(const MouseEvent&);
    virtual void on_keyboard_event(const KeyEvent&);
protected:
    using ViewList = std::vector<std::unique_ptr<IView>>;

    ViewList m_views;
};

} // namespace Slic3r::App::View
