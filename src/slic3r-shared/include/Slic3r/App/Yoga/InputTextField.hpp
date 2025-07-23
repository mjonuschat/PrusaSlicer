///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/InputText.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"

namespace Slic3r::App::Yoga {

class InputTextField : public Rectangle
{
public:
    explicit InputTextField(const std::string& name = "InputText");

    void process_events(Vec2f pos, Vec2f size) override;

    InputText::Callbacks& callbacks();

    /**
     * @note We assume UTF-8 encoding
     */
    const std::string& text() const;
    void set_text(const std::string& text);

    ImGuiInputTextFlags flags() const;
    void set_flags(ImGuiInputTextFlags flags);

    const std::string& hint() const;
    void set_hint(const std::string& hint);

    Validator* validator() const;
    void set_validator(std::unique_ptr<Validator> validator);

    void set_tooltip(const std::string& tooltip);
    void set_tooltip_position(Yoga::Position position);

    bool hovered() const;

protected:
    InputText* input_text() const;

protected:
    Tooltip m_tooltip;

private:
    InputText* m_input_text{nullptr};

    bool m_hovered = false;
};

} // namespace Slic3r::App::Yoga
