///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Tooltip;

class AbstractButton : public Item
{
public:
    // parameters for action functions is a bounding box of item
    struct Callbacks
    {
        std::function<void()> action{nullptr};
        std::function<void(bool hovered)> hovered_changed{nullptr};
        std::function<void(bool checked)> checked_changed{nullptr};
    };

    explicit AbstractButton(wchar_t icon, const std::string& tooltip = {});

    void process_events(Vec2f pos, Vec2f size) override;

    Callbacks& callbacks();

    const std::string& shortcut() const;
    void set_shortcut(const std::string& shortcut);

    bool has_arrow() const;
    void set_has_arrow(bool has_arrow);

    bool enabled() const;
    void set_enabled(bool new_enabled);

    bool checkable() const;
    void set_checkable(bool checkable);

    bool checked() const;
    void set_checked(bool checked);

    bool hovered() const;

protected:
    Tooltip* m_tooltip = nullptr;

    bool m_has_arrow = false;
    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_hovered = false;
    wchar_t m_icon = '\0';

    std::string m_shortcut;
    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Yoga
