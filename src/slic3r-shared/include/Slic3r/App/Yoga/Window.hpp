///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Window : public Item
{
public:
    struct Callbacks
    {
        std::function<void()> set_dirty_requested{nullptr};
        std::function<void(bool hovered)> hovered_changed{nullptr};
    };

    explicit Window(const std::string& name);

    float rounding() const;
    void set_rounding(float rounding);

    int flags() const;
    void set_flags(int flags);

    float alpha() const;
    void set_alpha(float alpha);

    void render(Vec2f pos, Vec2f size) override final;
    /**
     * @brief render_body by default will render all Window children
     * but you can override it and process the children yourself
     */
    virtual void render_body(Vec2f pos, Vec2f size);

    bool is_in_window() const override;

    void process_events(Vec2f pos, Vec2f size) override;

    bool position_by_yoga() const;
    void set_position_by_yoga(bool position_by_yoga);
    void request_position(Vec2f position);
    void bring_to_front();

    Callbacks& callbacks();

    bool hovered() const;

    bool is_modal() const;
    void set_modal(bool is_modal);

protected:
    void set_style_dirty() override;
    Vec2f get_item_size() override;

private:
    Callbacks m_callbacks;

    int m_flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoFocusOnAppearing;
    float m_rounding = 5;

    float m_alpha = 1.f;

    bool m_position_by_yoga = true;
    std::optional<Vec2f> m_requested_position;
    Vec2f m_last_pos;
    bool m_hovered = false;
    bool m_requested_bring_to_front = false;

    bool m_is_modal{ false };
};

} // namespace Slic3r::App::Yoga
