///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/InputTextField.hpp"

#include "Slic3r/App/Yoga/Validator.hpp"
#include <fmt/format.h>

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

InputTextField::InputTextField(const std::string& name) : m_tooltip(this, "", "")
{
    set_padding(1);
    set_disabled_fill(ImColor(32, 32, 32));
    m_input_text = emplace_back<InputText>(name);
    m_input_text->set_flex_grow(1);

    callbacks().update_revert_button = [this]() {
        update_revert_button();
    };
}

void InputTextField::process_events(Vec2f pos, Vec2f size)
{
    ImRect button_bb(to_im(pos), to_im(pos + size));

    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max, false);
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (!m_tooltip.text().empty()) {
            m_hovered ? m_tooltip.open() : m_tooltip.close();
        }
    }

    set_fill(m_hovered || m_input_text->active() ? ImColor(60, 60, 60) : ImColor(41, 41, 41));

    Item::process_events(pos, size);
}

InputText::Callbacks& InputTextField::callbacks()
{
    return m_input_text->callbacks();
}

const std::string& InputTextField::text() const
{
    return m_input_text->text();
}

void InputTextField::set_text(const std::string& text)
{
    m_input_text->set_text(text);
}

ImGuiInputTextFlags InputTextField::flags() const
{
    return m_input_text->flags();
}

void InputTextField::set_flags(ImGuiInputTextFlags flags)
{
    m_input_text->set_flags(flags);
}

const std::string& InputTextField::hint() const
{
    return m_input_text->hint();
}

void InputTextField::set_hint(const std::string& hint)
{
    m_input_text->set_hint(hint);
}

Validator* InputTextField::validator() const
{
    return m_input_text->validator();
}

void InputTextField::set_validator(std::unique_ptr<Validator> validator)
{
    m_input_text->set_validator(std::move(validator));
}

void InputTextField::set_tooltip(const std::string& tooltip)
{
    m_tooltip.set_text(tooltip);
}

void InputTextField::set_tooltip_position(Position position)
{
    m_tooltip.set_preferred_position(position);
}

bool InputTextField::hovered() const
{
    return m_hovered;
}

InputText* InputTextField::input_text() const
{
    return m_input_text;
}

void InputTextField::set_default(double default_value)
{
    if (DoubleValidator* double_validator = dynamic_cast<DoubleValidator*>(validator());
        double_validator && double_validator->precision().has_value())
    {
        m_default_text = fmt::format("{1:.{0}f}", double_validator->precision().value(), default_value);
    } else {
        m_default_text = fmt::format("{:.10g}", default_value);
    }
    update_revert_button();
}

void InputTextField::set_default(const std::string& default_text)
{
    m_default_text = default_text;
    update_revert_button();
}

bool InputTextField::is_changed_value() const
{
    return m_input_text->text() != m_default_text;
}

void InputTextField::reset()
{
    m_input_text->set_text(m_default_text);

    if (callbacks().text_edited) {
        callbacks().text_edited();
    }
}
} // namespace Slic3r::App::Yoga
