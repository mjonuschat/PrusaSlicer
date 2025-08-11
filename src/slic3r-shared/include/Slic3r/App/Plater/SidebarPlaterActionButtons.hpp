#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class SidebarPlaterActionButtons :
    public SidebarActionButtons,
    public Biz::IStatusCacheChangedListener,
    public Biz::ISelectedBedInstancesChangedListener
{
public:
    SidebarPlaterActionButtons(Navigator* render_module_navigator);
    ~SidebarPlaterActionButtons();

    void on_init(Biz::ProjectInteractor* project_interactor) override;

    void on_status_cache_changed(const Domain::SlicingId id) override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;

private:
    Yoga::LayoutButton* m_button_slice = nullptr;

    void update_slice_button(const Biz::Scene::BedSelection& selection);
};

} // namespace Slic3r::App::Plater
