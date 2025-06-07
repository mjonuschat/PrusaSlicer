///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <string>

namespace Slic3r::App::Yoga {

class Slider;
class InputTextField;

/**
 * @brief The SliderWithInput class is a composite control combining an input field and a slider.
 * Use set_direction(YGDirectionLTR) to change the alignment of the internal controls.
 */
class SliderWithInput : public Item
{
public:
    struct Callbacks
    {
        std::function<void(float value)> value_changed{ nullptr };
    };

    explicit SliderWithInput(float begin, float end, float step = 1.f);

    Callbacks& callbacks();

    void set_input_width(float width);
    void set_input_width_percent(float width_percent);

    float value() const;
    void set_value(float value);

    float begin() const;
    void set_begin_value(float begin);

    float end() const;
    void set_end_value(float end);

    float step() const;
    void set_step(float step);

private:
    Slider* m_slider = nullptr;
    InputTextField* m_input = nullptr;

    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Yoga
