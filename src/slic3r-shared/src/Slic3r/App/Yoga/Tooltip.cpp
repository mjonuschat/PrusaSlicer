///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Tooltip.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/Assert.hpp"

#include "imgui_internal.h"

namespace Slic3r::App::Yoga {

Tooltip::Tooltip(const std::string& window_name, const std::string& text, const std::string& shortcut)
    : AttachedWindow(window_name, Position::Right)
{
    set_orientation(Orientation::Horizontal);
    set_flags(
        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove
    );

    set_gap(5);
    set_padding(4);
    m_text = emplace_back<Text>(text);
    m_text->set_visible(!text.empty());
    m_shortcut = emplace_back<Text>(shortcut);
    m_shortcut->set_visible(!shortcut.empty());
    m_shortcut->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
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
