#include "Slic3r/App/SidebarActionButtons.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using RMType = Render::ModuleType;

SidebarActionButtons::SidebarActionButtons(
    const std::string& name,
    Render::ModuleType type,
    Navigator* render_module_navigator
) :
    Yoga::Window(name),
    m_render_module_navigator(render_module_navigator),
    m_type(type)
{
    ASSERT(type != RMType::Undef);

    m_navigator_name    = m_type == RMType::Plater ? ">" : "<";
    m_navigator_tooltip = m_type == RMType::Plater ? "Show Preview" : "Back to Plater";
    m_navigate_to_type  = m_type == RMType::Plater ? RMType::Preview : RMType::Plater;

    set_min_size({220, 0});
    set_flex_shrink(0);
}

std::unique_ptr<Yoga::LayoutButton> SidebarActionButtons::get_navigation_button()
{
    auto result{
        std::make_unique<Yoga::LayoutButton>(m_navigator_name, Render::Icon::None, m_navigator_tooltip)
    };
    result->set_background_color(color_secondary);
    result->set_label_font_type(Render::ImguiFontType::Bold);
    result->set_min_size({navig_btn_width, button_height});

    result->callbacks().action = [this]() { navigate_to_other(); };
    return result;
}

Domain::SlicingId SidebarActionButtons::active_bed_slicing_id() const
{
    return {
        m_project_interactor->selected_project_id(),
        m_project_interactor->scene_interactor().bed_selection().last_selected_bed().instance_id
    };
}

void SidebarActionButtons::navigate_to_other()
{
    m_render_module_navigator->set_render_module_type(m_navigate_to_type);
}

} // namespace Slic3r::App
