#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

#include <unordered_map>

namespace Slic3r::App::Yoga {

class Window : public Item
{
public:
    Window(const std::string& name, Item* parent = nullptr);
    virtual ~Window();

    const std::string& window_name() const;
    void set_window_name(const std::string& prefix);

    float rounding() const;
    void set_rounding(float rounding);

    int flags() const;
    void set_flags(int flags);

    void render(Vec2f pos, Vec2f size) override final;
    /**
     * @brief render_body by default will render all Window children
     * but you can override it and process the children yourself
     */
    virtual void render_body(Vec2f pos, Vec2f size);

protected:
    Vec2f get_item_size() override;

private:
    std::string m_window_name;

    static std::unordered_map<std::string, int> window_names;

    int m_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing;
    float m_rounding = 5;

};

} // namespace Slic3r::App::Yoga
