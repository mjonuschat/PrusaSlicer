#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"

#include "Slic3r/App/Yoga/ToggleButton.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Preview {
using Biz::ProjectInteractor;
using Biz::Slicing::SlicingInteractor;

SidebarAutoReslice::SidebarAutoReslice(ProjectInteractor& project_interactor) :
    Window("sidebar_auto_reslice")
{
    set_min_size({220, 0});

    Item* row = emplace_back<Yoga::Item>();
    row->set_orientation(Yoga::Orientation::Horizontal);

    m_auto_reslice_chb = row->emplace_back<Yoga::ToggleButton>(
        Biz::L("Auto-reslice"),
        Biz::L("Automatically runs slicing after any settings change.")
    );
    m_auto_reslice_chb->set_font_type(Render::ImguiFontType::Bold);
    m_auto_reslice_chb->callbacks().checked_changed = [&](bool checked)
    {
        SlicingInteractor& slicing_interactor{project_interactor.slicing_interactor()};
        if (checked) {
            slicing_interactor.enable_auto_slicing(project_interactor.selected_bed_slicing_id());
        } else {
            slicing_interactor.disable_auto_slicing();
        }
    };
}

bool SidebarAutoReslice::is_enabled() const
{
    return m_auto_reslice_chb->checked();
}

} // namespace Slic3r::App::Preview
