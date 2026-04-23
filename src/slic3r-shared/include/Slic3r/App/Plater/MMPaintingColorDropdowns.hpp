#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/ContextPopup.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Plater/MMPaintingScaleHelpers.hpp"
#include "Slic3r/App/Plater/MMPaintingUtils.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Domain/Color.hpp"

#include <functional>
#include <string>
#include <vector>

namespace Slic3r::App::Plater {

using Biz::_u8L;

class ColorMenuItem : public Yoga::RectangleButton
{
public:
    ColorMenuItem(
        const std::string& label,
        const Domain::ColorRGBA& color,
        bool selectable,
        bool dropdown_indicator = false
    );

    void set_entry(const std::string& label, const Domain::ColorRGBA& color);

    void render(Yoga::Vec2f pos, Yoga::Vec2f size) override;

private:
    Yoga::Circle* m_swatch{nullptr};
    std::string m_label;
    bool m_dropdown_indicator{false};
};

class ColorDropdown : public Yoga::Item
{
public:
    ColorDropdown(Render::Icon icon, const ImColor& color, float spacing);

    void
    set_items(const std::vector<std::string>& names, const std::vector<Domain::ColorRGBA>& colors);

    void set_current_index(std::size_t index);

    std::size_t current_index() const;

    void style_node() override;

    std::function<void(std::size_t index)> on_color_selected{[](std::size_t index) {}};

private:
    void rebuild_popup_items();
    void update_trigger_label();

    std::vector<std::string> m_names;
    std::vector<Domain::ColorRGBA> m_colors;
    std::size_t m_current_index{0};
    ColorMenuItem* m_trigger{nullptr};
    Yoga::ContextPopup* m_popup{nullptr};
    std::vector<ColorMenuItem*> m_popup_items;
};

class ColorDropdowns : public Yoga::Item
{
public:
    ColorDropdowns(float spacing, ImColor mouse_left_color, ImColor mouse_right_color)
    {
        set_orientation(Yoga::Orientation::Horizontal);
        auto dropdowns{emplace_back<Item>()};
        dropdowns->set_orientation(Yoga::Orientation::Vertical);
        dropdowns->set_flex_grow(1);
        dropdowns->set_gap(spacing);

        m_primary_dropdown =
            dropdowns->emplace_back<ColorDropdown>(Render::Icon::MouseLeft, mouse_left_color, spacing);
        m_primary_dropdown->on_color_selected = [this](std::size_t index)
        { on_color_selected(SelectedColor::Primary, index); };

        m_secondary_dropdown =
            dropdowns->emplace_back<ColorDropdown>(Render::Icon::MouseRight, mouse_right_color, spacing);
        m_secondary_dropdown->on_color_selected = [this](std::size_t index)
        { on_color_selected(SelectedColor::Secondary, index); };

        auto switch_area{emplace_back<Item>()};
        switch_area->set_align_items(YGAlign::YGAlignCenter);
        switch_area->set_justify_content(YGJustify::YGJustifyCenter);
        auto switch_button{
            switch_area->emplace_back<Yoga::LayoutButton>("", Render::Icon::Switch, _u8L("Switch colors"))
        };
        switch_button->set_content_padding(3_px);
        switch_button->set_width(22_px);
        switch_button->set_height(22_px);
        switch_button->set_margin(Yoga::Margins{spacing, 0, 0, 0});

        switch_button->callbacks().action = [this]()
        {
            switch_colors();
            on_color_selected(SelectedColor::Primary, m_primary_dropdown->current_index());
            on_color_selected(SelectedColor::Secondary, m_secondary_dropdown->current_index());
        };
    }

    void
    set_items(const std::vector<std::string>& names, const std::vector<Domain::ColorRGBA>& colors)
    {
        m_primary_dropdown->set_items(names, colors);
        m_secondary_dropdown->set_items(names, colors);
    }

    void select_color_index(SelectedColor color, std::size_t index)
    {
        if (color == SelectedColor::Primary) {
            m_primary_dropdown->set_current_index(index);
        } else if (color == SelectedColor::Secondary) {
            m_secondary_dropdown->set_current_index(index);
        } else {
            PANIC("Invalid color!");
        }
    }

    void switch_colors()
    {
        const std::size_t primary_index{m_primary_dropdown->current_index()};
        m_primary_dropdown->set_current_index(m_secondary_dropdown->current_index());
        m_secondary_dropdown->set_current_index(primary_index);
    }

    std::pair<std::size_t, std::size_t> current_indicies() const
    {
        return {m_primary_dropdown->current_index(), m_secondary_dropdown->current_index()};
    }

    std::function<void(SelectedColor color, std::size_t index)> on_color_selected{
        [](SelectedColor color, std::size_t index) {}
    };

private:
    ColorDropdown* m_primary_dropdown{nullptr};
    ColorDropdown* m_secondary_dropdown{nullptr};
};

}
