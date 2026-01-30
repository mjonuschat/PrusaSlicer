#pragma once

#include "Slic3r/App/Yoga/LayerHeightProfileControl.hpp"

#include <functional>
#include <imgui/imgui.h>

namespace Slic3r::App::Yoga {

class VariableLayerHeightControl : public LayerHeightProfileControl
{
public:
    enum class Button
    {
        None,
        Left,
        Right
    };

    struct Callbacks
    {
        std::function<void(std::optional<float> cursor_normalized_position)> on_mouse_move =
            [](std::optional<float>) {};
        std::function<void(
            float cursor_normalized_position,
            bool shift_down,
            bool ctrl_down,
            Button mouse_button
        )>
            on_mouse_down = [](float, bool, bool, Button) {};
        std::function<void(
            float cursor_normalized_position,
            bool shift_down,
            bool ctrl_down,
            Button mouse_button
        )>
            on_mouse_drag                                    = [](float, bool, bool, Button) {};
        std::function<void(Button mouse_button)> on_mouse_up = [](Button) {};
        std::function<void(float mouse_wheel_delta, bool ctrl_down)> on_mouse_wheel = [](float,
                                                                                         bool) {};
    };

    VariableLayerHeightControl();

    Callbacks& callbacks();

    void set_cursor_band_width(float width);
    void set_cursor_normalized_position(float normalized_position);
    void reset_cursor_position();

    void render(Vec2f pos, Vec2f size) override;

private:
    void render_cursor(const Vec2f& pos, const Vec2f& size);
    void process_input(const Vec2f& pos, const Vec2f& size);

    float m_cursor_band_width = 0.f;

    std::optional<float> m_cursor_normalized_position;

    Callbacks m_callbacks;

    Button m_mouse_button_down = Button::None;
    bool m_was_active          = false;
    bool m_was_hovered         = false;
};

} // namespace Slic3r::App::Yoga
