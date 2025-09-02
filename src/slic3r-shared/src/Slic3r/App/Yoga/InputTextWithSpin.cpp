///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/App/Yoga/SpinButton.hpp"

#include "Slic3r/App/Yoga/Validator.hpp"
#include <fmt/format.h>

#include <imgui_internal.h>
#include <cmath>

namespace Slic3r::App::Yoga {

InputTextWithSpin::InputTextWithSpin(
    std::unique_ptr<Validator> validator_in,
    double step,
    double step_fast,
    const std::string& name
) :
    InputTextField(name),
    m_step(step),
    m_step_fast(step_fast)
{
    set_validator(std::move(validator_in));
    set_orientation(Orientation::Horizontal);

    InputTextField::callbacks().text_edited = [this]() {
        try {
            m_last_value = boost::get<double>(m_eval.eval(m_parser.parse(text())));
        } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
        } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
        }

        if (m_callbacks.text_edited) {
            m_callbacks.text_edited();
        }
    };

    set_padding(0);
    Item* spins = emplace_back<Item>();
    spins->set_gap(2);
    spins->set_orientation(Orientation::Vertical);
    spins->set_justify_content(YGJustifyCenter);
    spins->set_padding(Paddings(0, 0, 4, 0));

    m_increase_button                     = spins->emplace_back<SpinButton>(ImGuiDir_Up);
    m_increase_button->callbacks().action = [this]() -> void {
        increase_value();
    };
    m_increase_button->set_width(10);
    m_increase_button->set_height(10);

    m_decrease_button                     = spins->emplace_back<SpinButton>(ImGuiDir_Down);
    m_decrease_button->callbacks().action = [this]() -> void {
        decrease_value();
    };
    m_decrease_button->set_width(10);
    m_decrease_button->set_height(10);
}

InputTextWithSpin::Callbacks& InputTextWithSpin::callbacks()
{
    return m_callbacks;
}

double InputTextWithSpin::step()
{
    return m_step;
}

double InputTextWithSpin::step_fast()
{
    return m_step_fast;
}

void InputTextWithSpin::set_step(double step)
{
    m_step = step;
}

void InputTextWithSpin::set_step_fast(double step_fast)
{
    m_step_fast = step_fast;
}

void InputTextWithSpin::set_default(double default_value)
{
    InputTextField::set_default(default_value);
}

void InputTextWithSpin::increase_value()
{
    m_last_value += GImGui->IO.KeyCtrl ? m_step_fast : m_step;
    set_text(fmt::format("{:.12g}", m_last_value));
}

void InputTextWithSpin::decrease_value()
{
    m_last_value -= GImGui->IO.KeyCtrl ? m_step_fast : m_step;
    set_text(fmt::format("{:.12g}", m_last_value));
}

} // namespace Slic3r::App::Yoga
