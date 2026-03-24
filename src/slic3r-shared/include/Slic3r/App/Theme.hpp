///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/App/Platform/AbstractTheme.hpp>

#include <unordered_map>
#include <memory>

namespace Slic3r::App {

class Theme : public Platform::AbstractTheme
{
public:
    Theme();

    const Domain::ColorRGBA& color(
        Platform::Color color_id,
        Platform::ColorGroup group_id = Platform::ColorGroup::Default
    ) const override;
    const ImColor& color_imgui(
        Platform::Color color_id,
        Platform::ColorGroup group_id = Platform::ColorGroup::Default
    ) const override;

    void set_style(Style style) override;

    void initialize_imgui_style() override;

private:
    struct ColorEntry
    {
        const Domain::ColorRGBA& color(Platform::ColorGroup group) const;
        const ImColor& color_imgui(Platform::ColorGroup group) const;

        ImColor color_default;
        std::unique_ptr<ImColor> color_disabled; ///< may be null
        std::unique_ptr<ImColor> color_hovered; ///< may be null
        std::unique_ptr<ImColor> color_active; ///< may be null
        std::unique_ptr<ImColor> color_active_disabled; ///< may be null
    };

    /**
     * @note Consider this helper only temporary, we should aim to declare all colors manually
     */
    ColorEntry auto_entry(const ImColor& color) const;

private:
    std::unordered_map<Platform::Color, ColorEntry> m_colors;
};

} // namespace Slic3r::App
