#pragma once

#include "Slic3r/App/Yoga/GizmoWindow.hpp"

#include <string>

namespace Slic3r::App::Yoga {

class Rectangle;

/**
 * @brief A GizmoWindow variant that supports a side panel on the left.
 */
class GizmoWindowWithLeftSidePanel : public GizmoWindow
{
public:
    explicit GizmoWindowWithLeftSidePanel(const std::string& title, Render::Icon icon);

    Item* side_panel_content() const;
    void set_side_panel_background_color(ImColor color);
    void set_side_panel_header_title(const std::string& label);

private:
    Item* m_side_panel                 = nullptr;
    Rectangle* m_side_panel_background = nullptr;
    Item* m_side_panel_header          = nullptr;
    Text* m_side_panel_header_title    = nullptr;
    Item* m_side_panel_content         = nullptr;
};

} // namespace Slic3r::App::Yoga
