#pragma once

#include "Slic3r/App/Plater/MMPaintingUtils.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/Yoga/TwoColorRing.hpp"
#include "Slic3r/App/Plater/MMPaintingScaleHelpers.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Plater {

class ColorButton : public Yoga::AbstractButton
{
public:
    ColorButton(
        const std::string& label,
        const Domain::ColorRGBA& color,
        ImColor mouse_left_color,
        ImColor mouse_right_color
    );

    void action_internal() override;

    void secondary_action_internal() override;

    std::function<void(SelectedColor)> on_color_selected{[](SelectedColor) {}};

    void select_color(SelectedColor color);

    bool primary_selected() const;

    bool secondary_selected() const;

    void clear_color(SelectedColor color);

private:
    void update_highlight_circle();

    ImColor m_mouse_left_color;
    ImColor m_mouse_right_color;
    Yoga::TwoColorRing* m_highlight_circle{nullptr};
    SelectedColor m_selected_color{SelectedColor::None};
};

class ColorSelector : public Yoga::Item
{
public:
    ColorSelector(ImColor mouse_left_color, ImColor mouse_right_color);

    void set_colors(const std::vector<Domain::ColorRGBA>& colors);

    void select_color_index(SelectedColor color, std::size_t index);

    std::size_t colors_count() const;

    std::function<void(SelectedColor, std::size_t)> on_color_selected{[](SelectedColor,
                                                                         std::size_t) {}};

private:
    std::vector<ColorButton*> m_selectors;
    ImColor m_mouse_left_color;
    ImColor m_mouse_right_color;
};

} // namespace Slic3r::App::Plater
