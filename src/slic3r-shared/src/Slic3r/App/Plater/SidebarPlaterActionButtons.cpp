#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

namespace Slic3r::App::Plater {

SidebarPlaterActionButtons::SidebarPlaterActionButtons(Item* parent)
    : SidebarActionButtons("sidebar_plater_action_buttons", Render::ModuleType::Plater, parent)
{
    m_button_slice = new Yoga::LayoutButton("Slice", this);
    m_button_slice->set_flex_grow(1);
    m_button_slice->set_background_color(color_primary);
    m_button_slice->set_min_size({0, button_height});
    m_button_slice->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_slice->callbacks().action = [this]() {
        m_project_interactor->slicing_interactor().slice_bed(active_bed_slicing_id().bed_instance_id
        );
        navigate_to_other();
    };
}

} // namespace Slic3r::App::Plater
