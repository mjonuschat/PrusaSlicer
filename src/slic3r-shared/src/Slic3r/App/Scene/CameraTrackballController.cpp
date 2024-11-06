#include "Slic3r/App/Scene/CameraTrackballController.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App::Scene {

float period_normalized(float val, float period_min, float period_max) {
    float fract;
    const float amplitude = period_max - period_min;

    std::modf((val - period_min) / amplitude, &fract);
    return period_min + amplitude * fract;
}

void CameraTrackballController::update_camera()
{
    Vec3f off{
        m_cam_focal_dist * std::sinf(m_zenith) * std::cosf(m_azimuth),
        m_cam_focal_dist * std::sinf(m_zenith) * std::sinf(m_azimuth),
        m_cam_focal_dist * std::cosf(m_zenith)
    };
    Vec3f pos = m_cam_focal - off;
    Vec3f up {
        std::cosf(m_zenith) * std::cosf(m_azimuth),
        std::cosf(m_zenith) * std::sinf(m_azimuth),
        -std::sinf(m_zenith)
        // 0, 0, 1
    };

    //SPDLOG_INFO("focal point: ({} {} {})", m_cam_focal.x(), m_cam_focal.y(), m_cam_focal.z());
    m_camera.look_at(pos, m_cam_focal, up);
}


void CameraTrackballController::set_zenith(float value)
{
    m_zenith = period_normalized(value, -M_PI, M_PI);
    normalize_azimuth_and_zenith();
    update_camera();
}

void CameraTrackballController::normalize_azimuth_and_zenith()
{
    // Normalize zenith to the range -M_PI_2 to M_PI_2
    m_zenith = fmodf(m_zenith, 2.0 * M_PI);
    if (m_zenith < 0) {
        m_zenith += 2 * M_PI;
    } else if (m_zenith > 2 * M_PI) {
        m_zenith = 2 * M_PI - m_zenith;
        m_azimuth += M_PI;
    }

    m_azimuth = fmodf(m_azimuth, 2.0 * M_PI);
    if (m_azimuth < 0) {
        m_azimuth += 2.0 * M_PI;
    }
}

} // namespace Slic3r::App::Scene
