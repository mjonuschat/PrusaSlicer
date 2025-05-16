#include "Slic3r/App/SidebarActionButtons.hpp"

#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using RMType = Render::ModuleType;

SidebarActionButtons::SidebarActionButtons(const std::string& name, Render::ModuleType type)
    : Yoga::Window(name), m_type(type)
{
    ASSERT(type != RMType::Undef);

    m_navigator_name = m_type == RMType::Plater ? ">" : "<";
    m_navigator_tooltip = m_type == RMType::Plater ? "Show Preview" : "Back to Plater";
    m_navigate_to_type = m_type == RMType::Plater ? RMType::Preview : RMType::Plater;

    set_min_size({220, 0});
}

void SidebarActionButtons::on_init(Biz::ProjectInteractor* project_interactor)
{
    m_project_interactor = project_interactor;
}

Biz::Slicing::SlicingId SidebarActionButtons::active_bed_slicing_id() const
{
    return {
        m_project_interactor->selected_project_id(),
        m_project_interactor->scene_interactor().selected_bed_instance().instance_id
    };
}

bool SidebarActionButtons::slice_allowed() const
{
    const Biz::Slicing::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status{
        m_project_interactor->status_cache().get_status(id)
    };

    return status && status == Biz::Slicing::Status::Modified;
}

void SidebarActionButtons::navigate_to_other()
{
    invoke_listeners<IRenderModuleChangedListener>([this](auto* listener) {
        listener->on_render_module_changed(m_navigate_to_type);
    });
}

} // namespace Slic3r::App
