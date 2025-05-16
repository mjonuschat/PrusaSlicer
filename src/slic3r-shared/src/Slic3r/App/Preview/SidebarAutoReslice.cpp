#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"

#include "Slic3r/App/I18N/I18N.hpp"

namespace Slic3r::App::Preview {

SidebarAutoReslice::SidebarAutoReslice() : Window("sidebar_auto_reslice")
{
    set_min_size({ 220, 0 });

    Item* row = emplace_back<Yoga::Item>();
    row->set_orientation(Yoga::Orientation::Horizontal);

    m_auto_reslice_chb = row->emplace_back<Yoga::ToggleButton>(L("Auto-reslice"), L("Some tt"));
    m_auto_reslice_chb->set_font_type(Render::ImguiFontType::Bold);
    m_auto_reslice_chb->callbacks().checked_changed = [](bool checked) {
        // Todo enable/disable autoreslicing for the Application
    };
}

} // namespace Slic3r::App::Preview
