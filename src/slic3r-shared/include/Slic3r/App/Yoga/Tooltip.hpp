#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {

class Tooltip : public Window
{
public:
    Tooltip(
        const std::string& window_name,
        const std::string& text,
        const std::string& shortcut,
        Item* parent
    );

    void render_body(Vec2f pos, Vec2f size) override;

    void style_node() override;

    const std::string& text() const;
    void set_text(const std::string& text);

    const std::string& shortcut() const;
    void set_shortcut(const std::string& shortcut);

private:
    std::string m_text;
    std::string m_shortcut;
};

} // namespace Slic3r::App::Yoga
