///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/Slider.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "libslic3r/format.hpp"

namespace Slic3r::App::Yoga {

SliderWithInput::Callbacks& SliderWithInput::callbacks() { return m_callbacks; }

SliderWithInput::SliderWithInput(float begin, float end, float step) : Item()
{
    set_gap(10.f);
    set_align_items(YGAlignCenter);

    m_input = emplace_back<InputTextField>(Slic3r::format("%1%", begin));
    m_input->set_flags(ImGuiInputTextFlags_CharsDecimal);
    m_input->callbacks().text_edited = [this]() {
        m_slider->set_value(std::stof(m_input->text())); };

    m_slider = emplace_back<Slider>(begin, end, step);
    m_slider->set_flex_grow(1);
    m_slider->callbacks().value_changed = [this](float value) {
        m_input->set_text(Slic3r::format("%1%", value));
        if (callbacks().value_changed)
            callbacks().value_changed(value);
    };

    m_input->set_text(Slic3r::format("%1%", m_slider->begin()));
}

void SliderWithInput::set_input_width(float width)
{
    m_input->set_width(width);
}

void SliderWithInput::set_input_width_percent(float width_percent)
{
    m_input->set_width_percent(width_percent);
}

float SliderWithInput::value() const { return m_slider->value(); }
void SliderWithInput::set_value(const float value) { m_slider->set_value(value); }

float SliderWithInput::begin() const { return m_slider->begin(); }
void SliderWithInput::set_begin_value(float begin) { m_slider->set_begin_value(begin); }

float SliderWithInput::end() const { return m_slider->end(); }
void SliderWithInput::set_end_value(float end) { m_slider->set_end_value(end); }

float SliderWithInput::step() const { return m_slider->step(); }
void SliderWithInput::set_step(float step) { m_slider->set_step(step); }

} // namespace Slic3r::App::Yoga
