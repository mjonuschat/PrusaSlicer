///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"

namespace Slic3r::App::Yoga {

class AbstractButton : public Item
{
public:
    /**
     * @brief The Callbacks class - All actions are always consumed user-side
     */
    struct Callbacks
    {
        std::function<void()> action{nullptr};
        std::function<void()> secondary_action{nullptr};
        std::function<void(bool hovered)> hovered_changed{nullptr};
        std::function<void(bool pressed)> pressed_primary_changed{nullptr};
        std::function<void(bool pressed)> pressed_secondary_changed{nullptr};
        std::function<void(bool checked)> checked_changed{nullptr};
        std::function<bool()> enabled{nullptr};
    };

    AbstractButton(const std::string& tooltip = {}, const std::string& name = {});

    void render(Vec2f pos, Vec2f size) override;

    Callbacks& callbacks();

    const std::string& shortcut() const;
    void set_shortcut(const std::string& shortcut);

    void set_tooltip(const std::string& tooltip);
    void set_tooltip_position(Yoga::Position position);

    bool has_arrow() const;
    void set_has_arrow(bool has_arrow);

    bool checkable() const;
    void set_checkable(bool checkable);

    bool checked() const;
    void set_checked(bool checked);

    bool hovered() const;

    bool pressed() const;

    bool allow_overlap() const;
    void set_allow_overlap(bool allow_overlap);

    ImGuiMouseCursor cursor() const;
    void set_cursor(ImGuiMouseCursor cursor);

    ImGuiMouseButton primary_button() const;
    void set_primary_button(ImGuiMouseButton primary_button);

    ImGuiMouseButton secondary_button() const;
    void set_secondary_button(ImGuiMouseButton secondary_button);

protected:
    virtual void checked_updated_internal();
    virtual void hovered_updated_internal();
    virtual void pressed_primary_updated_internal();
    virtual void pressed_secondary_updated_internal();
    virtual void action_internal();
    virtual void secondary_action_internal();
    virtual void set_shortcut_internal(const std::string& shortcut);

    void enabled_updated_internal() override;
    void visible_updated_internal() override;

    Platform::ColorGroup button_color_group() const;

private:
    void set_hovered(bool hovered);
    void set_pressed_primary(bool pressed);
    void set_pressed_secondary(bool pressed);
    void update_flags();

protected:
    Tooltip* m_tooltip = nullptr;

private:
    bool m_has_arrow         = false;
    bool m_checkable         = false;
    bool m_checked           = false;
    bool m_hovered           = false;
    bool m_pressed_primary   = false;
    bool m_pressed_secondary = false;
    bool m_allow_overlap     = false;

    std::string m_shortcut;
    Callbacks m_callbacks;
    ImGuiButtonFlags m_flags = ImGuiButtonFlags_MouseButtonLeft
        | ImGuiButtonFlags_MouseButtonRight
        | ImGuiButtonFlags_EnableNav;
    ImGuiMouseButton m_primary_button   = ImGuiMouseButton_Left;
    ImGuiMouseButton m_secondary_button = ImGuiMouseButton_Right;
    ImGuiMouseCursor m_cursor           = ImGuiMouseCursor_Arrow;
};

} // namespace Slic3r::App::Yoga
