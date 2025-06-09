///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/InputTextField.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

InputTextField::InputTextField(const std::string& name)
{
    set_padding(1);
    m_input_text = emplace_back<InputText>(name);
    m_input_text->set_flex_grow(1);
}

void InputTextField::process_events(Vec2f pos, Vec2f size)
{
    ImRect button_bb(to_im(pos), to_im(pos + size));

    m_hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max, false);

    set_fill(m_hovered || m_input_text->active() ? ImColor(60, 60, 60) : ImColor(41, 41, 41));

    Item::process_events(pos, size);
}

InputText::Callbacks& InputTextField::callbacks() { return m_input_text->callbacks(); }

const std::string& InputTextField::text() const { return m_input_text->text(); }

void InputTextField::set_text(const std::string& text) { m_input_text->set_text(text); }

ImGuiInputTextFlags InputTextField::flags() const { return m_input_text->flags(); }

void InputTextField::set_flags(ImGuiInputTextFlags flags) { m_input_text->set_flags(flags); }

const std::string& InputTextField::hint() const { return m_input_text->hint(); }

void InputTextField::set_hint(const std::string& hint) { m_input_text->set_hint(hint); }

Validator* InputTextField::validator() const { return m_input_text->validator(); }

void InputTextField::set_validator(std::unique_ptr<Validator> validator)
{
    m_input_text->set_validator(std::move(validator));
}

bool InputTextField::hovered() const { return m_hovered; }

InputText* InputTextField::input_text() const { return m_input_text; }
} // namespace Slic3r::App::Yoga
