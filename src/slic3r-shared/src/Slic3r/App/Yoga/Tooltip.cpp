///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Tooltip.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/Assert.hpp"

#include "imgui_internal.h"

namespace Slic3r::App::Yoga {

Tooltip::Tooltip(
    Item* parent, const std::string& text, const std::string& shortcut, const std::string& window_name
)
{
    WindowPtr window = std::make_unique<Window>(window_name);

    window->set_orientation(Orientation::Horizontal);
    window->set_flags(
        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove
    );

    window->set_gap(5);
    window->set_padding(4);
    m_text = window->emplace_back<Text>(text);
    m_text->set_visible(!text.empty());
    m_shortcut = window->emplace_back<Text>(shortcut);
    m_shortcut->set_visible(!shortcut.empty());
    m_shortcut->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);

    set_content_item(std::move(window));

    attach_to_item(parent);
}

const std::string& Tooltip::text() const { return m_text->text(); }

void Tooltip::set_text(const std::string& text)
{
    m_text->set_text(text);
    m_text->set_visible(!text.empty());
}

const std::string& Tooltip::shortcut() const { return m_shortcut->text(); }

void Tooltip::set_shortcut(const std::string& shortcut)
{
    m_shortcut->set_text(shortcut);
    m_shortcut->set_visible(!shortcut.empty());
}

} // namespace Slic3r::App::Yoga
