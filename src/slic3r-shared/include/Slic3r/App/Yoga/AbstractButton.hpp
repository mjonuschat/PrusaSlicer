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
    // parameters for action functions is a bounding box of item
    struct Callbacks
    {
        std::function<void()> action{nullptr}; ///< Always user action
        std::function<void(bool hovered)> hovered_changed{nullptr}; ///< Always user action
        std::function<void(bool pressed)> pressed_changed{nullptr}; ///< Always user action
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

    ImGuiButtonFlags flags() const;

    bool allow_overlap() const;
    void set_allow_overlap(bool allow_overlap);

    ImGuiMouseCursor cursor() const;
    void set_cursor(ImGuiMouseCursor cursor);

protected:
    virtual void checked_updated_internal();
    virtual void hovered_updated_internal();
    virtual void pressed_updated_internal();
    virtual void action_internal();
    virtual void set_shortcut_internal(const std::string& shortcut);

    void enabled_updated_internal() override;
    void visible_updated_internal() override;

private:
    void set_hovered(bool hovered);
    void set_pressed(bool pressed);

protected:
    Tooltip* m_tooltip = nullptr;

private:
    bool m_has_arrow     = false;
    bool m_checkable     = false;
    bool m_checked       = false;
    bool m_hovered       = false;
    bool m_pressed       = false;
    bool m_allow_overlap = false;

    std::string m_shortcut;
    Callbacks m_callbacks;
    ImGuiButtonFlags m_flags  = ImGuiButtonFlags_MouseButtonLeft;
    ImGuiMouseCursor m_cursor = ImGuiMouseCursor_Arrow;
};

} // namespace Slic3r::App::Yoga
