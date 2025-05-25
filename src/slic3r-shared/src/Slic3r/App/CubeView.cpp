#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App {

CubeView::CubeView() : Window("cube_view") {
    set_flags(flags() | ImGuiWindowFlags_NoBackground);
    set_min_size({70.f, 70.f});
    set_max_size({70.f, 70.f});
    set_aspect_ratio(1.0);
}

void CubeView::render_body(Domain::Vec2f pos, Domain::Vec2f size)
{
    // !!! temporary code. Needed tobe changed for real view cube
    Imgui::icon_button(Render::Icon::CubeViewIcon, to_im(size));
}

}// Slic3r::App namespace
