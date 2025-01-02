#include <cmath>

#include "Slic3r/App/Scene/CameraTrackballController.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App::Scene {

double period_normalized(double val, double period_min, double period_max) {
    double fract;
    const double amplitude = period_max - period_min;

    std::modf((val - period_min) / amplitude, &fract);
    return period_min + amplitude * fract;
}

void CameraTrackballController::update_camera()
{
    Vec3d off{
        m_cam_focal_dist * sinf(m_zenith) * cosf(m_azimuth),
        m_cam_focal_dist * sinf(m_zenith) * sinf(m_azimuth),
        m_cam_focal_dist * cosf(m_zenith)
    };
    Vec3d pos = m_cam_focal - off;
    Vec3d up {
        cosf(m_zenith) * cosf(m_azimuth),
        cosf(m_zenith) * sinf(m_azimuth),
        -sinf(m_zenith)
        // 0, 0, 1
    };

    //SPDLOG_INFO("focal point: ({} {} {})", m_cam_focal.x(), m_cam_focal.y(), m_cam_focal.z());
    m_camera.look_at(pos, m_cam_focal, up);
}


void CameraTrackballController::set_zenith(double value)
{
    m_zenith = period_normalized(value, -M_PI, M_PI);
    normalize_azimuth_and_zenith();
    update_camera();
}

void CameraTrackballController::normalize_azimuth_and_zenith()
{
    // Normalize zenith to the range -M_PI_2 to M_PI_2
    m_zenith = fmod(m_zenith, 2.0 * M_PI);
    if (m_zenith < 0) {
        m_zenith += 2 * M_PI;
    } else if (m_zenith > 2 * M_PI) {
        m_zenith = 2 * M_PI - m_zenith;
        m_azimuth += M_PI;
    }

    m_azimuth = fmod(m_azimuth, 2.0 * M_PI);
    if (m_azimuth < 0) {
        m_azimuth += 2.0 * M_PI;
    }
}

} // namespace Slic3r::App::Scene
