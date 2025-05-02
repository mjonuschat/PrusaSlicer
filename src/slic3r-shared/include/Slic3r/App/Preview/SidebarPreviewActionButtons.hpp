#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Preview {

class SidebarPreviewActionButtons : public SidebarActionButtons {
public:
    explicit SidebarPreviewActionButtons(Yoga::Item* parent = nullptr);

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

private:
    bool export_allowed() const;

    void render_export_buttons();
    void render_navigation_button();
    void render_slice_button(Domain::Vec2f size);
};

}
