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
        std::function<bool()> visibility{[]() { return true; }};
        std::function<bool()> enabled{[]() { return true; }};
        std::function<bool()> toggled{[]() { return false; }};
        std::function<void()> action_on_arrow{nullptr};
        std::function<void()> action_on_arrow_hovering{nullptr};

        bool is_empty() const { return !action && !action_on_arrow && !action_on_arrow_hovering; }
    };

    explicit AbstractButton(wchar_t icon, const std::string& tooltip = {}, Item* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;

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

protected:
    Tooltip* m_tooltip = nullptr;

    bool m_has_arrow = false;
    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
    wchar_t m_icon = '\0';

    std::string m_shortcut;
    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Yoga
