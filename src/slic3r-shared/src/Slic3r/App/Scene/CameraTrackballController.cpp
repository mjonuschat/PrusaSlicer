#include "Slic3r/App/Scene/CameraTrackballController.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App::Scene {

void CameraTrackballController::update_camera()
{
    Vec3f off{
        m_cam_focal_dist * std::sinf(m_zenith) * std::cosf(m_azimuth),
        m_cam_focal_dist * std::sinf(m_zenith) * std::sinf(m_azimuth),
        m_cam_focal_dist * std::cosf(m_zenith)
    };
    Vec3f pos = m_cam_focal - off;
    Vec3f up {
        //-std::cosf(m_zenith) * std::cosf(m_azimuth),
        //-std::cosf(m_zenith) * std::sinf(m_azimuth),
        //-std::sinf(m_zenith)
        0, 0, 1
    };

    //SPDLOG_INFO("focal point: ({} {} {})", m_cam_focal.x(), m_cam_focal.y(), m_cam_focal.z());
    m_camera.look_at(pos, m_cam_focal, up);
}


void CameraTrackballController::set_zenith(float value)
{
    m_zenith = value;
    clamp_zenith();
    update_camera();
}

void CameraTrackballController::clamp_zenith()
{
    if (m_zenith > M_PI - MIN_ZENITH) m_zenith = M_PI - MIN_ZENITH;
    else if (m_zenith < 0 + MIN_ZENITH) m_zenith = MIN_ZENITH;
}

}
