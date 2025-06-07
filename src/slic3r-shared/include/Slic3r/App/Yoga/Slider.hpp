#pragma once
#include "Slic3r/App/Yoga/Oval.hpp"

namespace Slic3r::App::Yoga {

class Circle;

class Slider : public Oval {
public:
    struct Callbacks
    {
        std::function<void(float value)> value_changed{ nullptr };
    };

    explicit Slider(float begin, float end, float step = 1.f);

    Callbacks& callbacks();

    float value() const;
    void set_value(float value);

    float begin() const;
    void set_begin_value(float begin);

    float end() const;
    void set_end_value(float end);

    float step() const;
    /**
     * @warning The step value must not be zero.
     */
    void set_step(float step);

private:
    void process_events(Vec2f pos, Vec2f size) override;
    void set_hovered(bool hovered);
    float clamp(float value);
    float snap_to_nearest(float value);

private:
    Oval* m_area{ nullptr };
    Circle* m_thumb{ nullptr };

    float m_begin_value{ 0.f };
    float m_end_value{ 0.f };
    float m_step{ 0.f };
    float m_value{ 0.f };

    bool m_dragging{ false };
    bool m_hovered{ false };

    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Yoga 