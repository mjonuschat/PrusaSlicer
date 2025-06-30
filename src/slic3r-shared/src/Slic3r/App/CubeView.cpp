#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"
#include "Slic3r/Domain/Transformation.hpp"
#include "Slic3r/App/Imgui/NavigationCube.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App {

static constexpr float SIZE = 80.0f;

CubeView::CubeView() : Window("cube_view") {
    set_flags(flags() | ImGuiWindowFlags_NoBackground);
    set_min_size({SIZE, SIZE});
    set_max_size({SIZE, SIZE});
    set_aspect_ratio(1.0);
}

void CubeView::render_body(Domain::Vec2f pos, Domain::Vec2f size)
{
    DEBUG_ASSERT(m_camera != nullptr);
    DEBUG_ASSERT(m_trackball != nullptr);

    Domain::SquareMatrix4f view = m_camera->view().cast<float>();
    Domain::SquareMatrix4f proj = m_camera->projection().cast<float>();
    float cam_distance = m_trackball->distance_to_target();
    ImVec2 im_pos = ImVec2(pos.x(), pos.y());
    ImVec2 im_size = ImVec2(size.x(), size.y());

    Imgui::NavCube::set_draw_list(ImGui::GetWindowDrawList());
    Imgui::NavCube::set_orthographic(m_camera->cam_projection().type() == Scene::CameraProjectionType::Orthographic);
    Imgui::NavCube::view_manipulate(view.data(), proj.data(), cam_distance, im_pos, im_size, 0);
    if (Imgui::NavCube::is_animation_running()) {
        auto [azimuth, zenith] = Imgui::NavCube::get_azimuth_and_zenith();
        m_trackball->set_azimuth_and_zenith(azimuth, zenith);
        // requires extra frames for animation
        m_require_render = true;
    }
    else
        m_require_render = false;
}

}// Slic3r::App namespace
