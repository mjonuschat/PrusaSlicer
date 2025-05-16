#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
}

namespace Slic3r::App::Preview {

class SidebarPreviewActionButtons : public SidebarActionButtons {
public:
    SidebarPreviewActionButtons();

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

private:
    bool export_allowed() const;

private:
    Yoga::Item* m_layout_top = nullptr;
    Yoga::Item* m_layout_bottom = nullptr;

    Yoga::LayoutButton* m_button_print = nullptr;
    Yoga::LayoutButton* m_button_navigation = nullptr;
    Yoga::LayoutButton* m_button_save_print = nullptr;
    Yoga::LayoutButton* m_button_save_print_to_flash = nullptr;
    Yoga::LayoutButton* m_button_save_print_to_local = nullptr;
    Yoga::LayoutButton* m_button_save_print_add_bookmark = nullptr;
};

}
