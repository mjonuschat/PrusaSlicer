///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Theme.hpp"

#include <Slic3r/Assert.hpp>

#include <imgui_internal.h>

#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App {

Theme::Theme()
{
    m_colors[Platform::Color::Transparent] = ColorEntry{0, 0, 0, 0};

    m_colors[Platform::Color::Text] = ColorEntry{
        {1.00f, 1.00f, 1.00f, 1.00f},
        std::make_unique<ImColor>(0.50f, 0.50f, 0.50f, 1.00f) // disabled
    };
    m_colors[Platform::Color::TextLink] = ColorEntry{
        {0.16f, 0.29f, 0.48f, 1.00f},
        std::make_unique<ImColor>(0.50f, 0.50f, 0.50f, 1.00f) // disabled
    };
    m_colors[Platform::Color::WindowBg]          = auto_entry({27, 27, 27});
    m_colors[Platform::Color::WindowBgAlternate] = auto_entry({41, 41, 41});

    m_colors[Platform::Color::Scrollbar] = ColorEntry{
        {0.31f, 0.31f, 0.31f, 1.00f},
        nullptr,
        std::make_unique<ImColor>(0.41f, 0.41f, 0.41f, 1.00f),
        std::make_unique<ImColor>(0.51f, 0.51f, 0.51f, 1.00f)
    };

    m_colors[Platform::Color::NavCursor] = ColorEntry{{0.26f, 0.59f, 0.98f, 1.00f}};

    m_colors[Platform::Color::Button] = ColorEntry{
        m_colors[Platform::Color::WindowBgAlternate].color_default,
        std::make_unique<ImColor>(
            Theme::color_imgui(Platform::Color::WindowBgAlternate, Platform::ColorGroup::Disabled)
        ),
        std::make_unique<ImColor>(
            Theme::color_imgui(Platform::Color::WindowBgAlternate, Platform::ColorGroup::Hovered)
        ),
        std::make_unique<ImColor>(54, 73, 117),
        std::make_unique<ImColor>(Theme::color_imgui(
            Platform::Color::WindowBgAlternate,
            Platform::ColorGroup::ActiveDisabled
        ))
    };

    m_colors[Platform::Color::RadioButtonBackground] = ColorEntry{
        {127, 127, 127},
        std::make_unique<ImColor>(Imgui::adjust_brightness({127, 127, 127}, 1.5)),
        std::make_unique<ImColor>(Imgui::adjust_brightness({127, 127, 127}, 1.2)),
        std::make_unique<ImColor>(217, 217, 217),
        std::make_unique<ImColor>(Imgui::adjust_brightness({127, 127, 127}, 1.7))
    };

    m_colors[Platform::Color::RadioButton] = ColorEntry{
        Theme::color_imgui(Platform::Color::WindowBgAlternate),
        std::make_unique<ImColor>(
            Imgui::adjust_brightness(Theme::color_imgui(Platform::Color::WindowBgAlternate), 1.5)
        ),
        std::make_unique<ImColor>(
            Theme::color_imgui(Platform::Color::WindowBgAlternate, Platform::ColorGroup::Hovered)
        ),
        std::make_unique<ImColor>(78, 128, 248),
        std::make_unique<ImColor>(
            Imgui::adjust_brightness(Theme::color_imgui(Platform::Color::WindowBgAlternate), 1.9)
        )
    };

    m_colors[Platform::Color::ButtonTransparent] = ColorEntry{
        Theme::color_imgui(Platform::Color::Transparent),
        std::make_unique<ImColor>(
            Theme::color_imgui(Platform::Color::WindowBgAlternate, Platform::ColorGroup::Disabled)
        ),
        std::make_unique<ImColor>(
            Theme::color_imgui(Platform::Color::WindowBgAlternate, Platform::ColorGroup::Hovered)
        ),
        std::make_unique<ImColor>(54, 73, 117),
        std::make_unique<ImColor>(Theme::color_imgui(
            Platform::Color::WindowBgAlternate,
            Platform::ColorGroup::ActiveDisabled
        ))
    };

    m_colors[Platform::Color::AccentPrimary]   = ColorEntry{{223, 93, 45}};
    m_colors[Platform::Color::AccentSecondary] = ColorEntry{{0.32f, 0.48f, 0.84f, 1.0f}};
    m_colors[Platform::Color::Error]           = ColorEntry{{0.79f, 0.18f, 0.14f, 1.0f}};
    m_colors[Platform::Color::Warning]          = ColorEntry{{232, 64, 64}};
    m_colors[Platform::Color::ModalWindowDimBg] = ColorEntry{{0.80f, 0.80f, 0.80f, 0.35f}};

    m_colors[Platform::Color::SceneBg] = ColorEntry{{88, 88, 88}};
}

const Domain::ColorRGBA& Theme::color(Platform::Color color_id, Platform::ColorGroup group_id) const
{
    return m_colors.at(color_id).color(group_id);
}

const ImColor& Theme::color_imgui(Platform::Color color_id, Platform::ColorGroup group_id) const
{
    return m_colors.at(color_id).color_imgui(group_id);
}

void Theme::set_style(Style style)
{
    // Does nothing, implement properly during Light mode add
}

void Theme::initialize_imgui_style()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors    = style->Colors;

    // IMPORTANT: If the color is also used outside ImGui internals, it nees to be defined
    // in the Theme color system

    colors[ImGuiCol_Text] = color_imgui(Platform::Color::Text);
    colors[ImGuiCol_TextDisabled] =
        color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled);
    colors[ImGuiCol_WindowBg]         = color_imgui(Platform::Color::WindowBg);
    colors[ImGuiCol_ChildBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]          = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]           = colors[ImGuiCol_WindowBg];
    colors[ImGuiCol_BorderShadow]     = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]          = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]          = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]        = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]    = color_imgui(Platform::Color::Scrollbar);
    colors[ImGuiCol_ScrollbarGrabHovered] =
        color_imgui(Platform::Color::Scrollbar, Platform::ColorGroup::Hovered);
    colors[ImGuiCol_ScrollbarGrabActive] =
        color_imgui(Platform::Color::Scrollbar, Platform::ColorGroup::Active);
    colors[ImGuiCol_CheckMark]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]       = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]           = color_imgui(Platform::Color::Transparent);
    colors[ImGuiCol_ButtonHovered] =
        color_imgui(Platform::Color::Button, Platform::ColorGroup::Hovered);
    colors[ImGuiCol_ButtonActive] =
        color_imgui(Platform::Color::Button, Platform::ColorGroup::Active);
    colors[ImGuiCol_Header]            = ImVec4(0.21f, 0.29f, 0.46f, 0.31f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Separator]         = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]  = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]   = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]        = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected] =
        ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected] =
        ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]                 = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]             = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] =
        ImVec4(0.31f, 0.31f, 0.35f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] =
        ImVec4(0.23f, 0.23f, 0.25f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]              = color_imgui(Platform::Color::TextLink);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]             = color_imgui(Platform::Color::NavCursor);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = color_imgui(Platform::Color::ModalWindowDimBg);
}

Theme::ColorEntry Theme::auto_entry(const ImColor& color) const
{
    return {
        color,
        std::make_unique<ImColor>(Imgui::adjust_brightness(color, 0.8)),
        std::make_unique<ImColor>(Imgui::adjust_brightness(color, 1.2)),
        std::make_unique<ImColor>(Imgui::adjust_brightness(color, 1.5)),
        std::make_unique<ImColor>(Imgui::adjust_brightness(color, 1.7)),
    };
}

const Domain::ColorRGBA& Theme::ColorEntry::color(Platform::ColorGroup group) const
{
    return reinterpret_cast<const Domain::ColorRGBA&>(color_imgui(group));
}

const ImColor& Theme::ColorEntry::color_imgui(Platform::ColorGroup group) const
{
    switch (group) {
    case Platform::ColorGroup::Default:
        return color_default;
    case Platform::ColorGroup::Disabled:
        return color_disabled ? *color_disabled.get() : color_default;
    case Platform::ColorGroup::Active:
        return color_active ? *color_active.get() : color_default;
    case Platform::ColorGroup::ActiveDisabled:
        return color_active_disabled ? *color_active_disabled.get() : color_default;
    case Platform::ColorGroup::Hovered:
        return color_hovered ? *color_hovered.get() : color_default;
    }

    PANIC("ColorGroup is not handled!");
}

} // namespace Slic3r::App
