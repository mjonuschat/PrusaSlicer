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

protected:
    Vec2f get_item_size() override;

private:

    int m_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing;
    float m_rounding = 5;

    float m_alpha = 1.f;

};

} // namespace Slic3r::App::Yoga
