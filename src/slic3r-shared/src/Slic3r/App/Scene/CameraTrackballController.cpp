#include <cmath>

#include "Slic3r/App/Scene/CameraTrackballController.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App::Scene {

void CameraTrackballController::add_azimuth_and_zenith(double delta_azimuth, double delta_zenith, bool apply_limits)
{
    delta_zenith = fmod(delta_zenith, 2.0 * M_PI);
    m_zenith += delta_zenith;
    if (apply_limits) {
        if (m_zenith < 0.0) {
            delta_zenith -= m_zenith;
            m_zenith = 0.0;
        }
        else if (m_zenith > M_PI) {
            delta_zenith -= m_zenith - M_PI;
            m_zenith = M_PI;
        }
    }

    delta_azimuth = fmod(delta_azimuth, 2.0 * M_PI);
    m_azimuth += delta_azimuth;
    if (m_azimuth < 0.0)
        m_azimuth += 2.0 * M_PI;
    else if (m_azimuth > 2.0 * M_PI)
        m_azimuth -= 2.0 * M_PI;

    Transform3d view = Transform3d(m_camera.view());
    Vec3d translation = view.translation() + m_view_rotation * m_pivot;
    Vec3d old_eye_target = view * m_target;

    auto rot_z = Eigen::AngleAxisd(delta_azimuth, Vec3d::UnitZ());
    m_view_rotation *= rot_z * Eigen::AngleAxisd(delta_zenith, rot_z.inverse() * m_camera.right());
    m_view_rotation.normalize();

    view.fromPositionOrientationScale(m_view_rotation * (-m_pivot) + translation, m_view_rotation, Vec3d::Ones());

    Transform3d model = view.inverse();
    m_camera.set_model(model.matrix());
    m_target = model * old_eye_target;
}

void CameraTrackballController::set_camera_orientation()
{
    double cos_a = cos(m_azimuth);
    double sin_a = sin(m_azimuth);
    double cos_z = cos(m_zenith);
    double sin_z = sin(m_zenith);

    Vec3d xyz = { sin_z * cos_a, sin_z * sin_a, cos_z };
    Vec3d off = m_distance * xyz;
    Vec3d up = (std::abs(to_2d(xyz).norm()) > EPSILON) ? Vec3d::UnitZ() :
        (std::abs(m_zenith) < EPSILON) ? Vec3d(-cos_a, -sin_a, 0.0f) : Vec3d(cos_a, sin_a, 0.0f);
    Vec3d pos = m_target - off;

    m_camera.look_at(pos, m_target, up);
    m_view_rotation = Eigen::Quaterniond(m_camera.view().block<3, 3>(0, 0));
}

void CameraTrackballController::normalize_azimuth_and_zenith()
{
    m_zenith = std::clamp(fmod(m_zenith, 2.0 * M_PI), 0.0, M_PI);
    m_azimuth = fmod(m_azimuth, 2.0 * M_PI);
    if (m_azimuth < 0.0)
        m_azimuth += 2.0 * M_PI;
}

} // namespace Slic3r::App::Scene
