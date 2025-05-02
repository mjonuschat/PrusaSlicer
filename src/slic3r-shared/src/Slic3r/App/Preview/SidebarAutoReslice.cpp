#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"

namespace Slic3r::App::Preview {

SidebarAutoReslice::SidebarAutoReslice(Item* parent) : Window("sidebar_auto_reslice", parent)
{
    set_min_size({220, 30});
}

void SidebarAutoReslice::render_body(Domain::Vec2f pos, Domain::Vec2f size)
{
    static bool check{true};
    ImGui::Checkbox("Auto re-slice", &check);
}

} // namespace Slic3r::App::Preview
