#pragma once

#include "Slic3r/App/Scene/Camera.hpp"

namespace Slic3r::App::Scene {

static constexpr double DEFAULT_AZIMUTH = M_PI_4;
static constexpr double DEFAULT_ZENITH = 3.0 * M_PI_4;

class CameraTrackballController
{
public:
  explicit CameraTrackballController(Camera& camera) : m_camera(camera) { set_camera_orientation(); }

    void set_target(const Vec3d& pos)
    {
        m_target = pos;
        m_camera.look_at(m_target - m_distance * m_camera.forward(), m_target, m_camera.up());
    }

    void set_distance_to_target(double value)
    {
        m_distance = std::max(MIN_FOCAL_DISTANCE, value);
        m_camera.look_at(m_target - m_distance * m_camera.forward(), m_target, m_camera.up());
    }

    void set_pivot(const Vec3d& pos) { m_pivot = pos; }
    void synchronize_pivot_with_target() { m_pivot = m_target; }

    void set_zoom(double value) { m_camera.set_zoom(value); }
    void update_zoom(double value) { m_camera.update_zoom(value); }
    void switch_projection_type() { m_camera.switch_projection_type(); }

    void set_azimuth_and_zenith(double azimuth, double zenith)
    {
        m_azimuth = azimuth;
        m_zenith = zenith;
        normalize_azimuth_and_zenith();
        set_camera_orientation();
    }

    void add_azimuth_and_zenith(double delta_azimuth, double delta_zenith, bool apply_limits = false);

    const Vec3d& target() const { return m_target; }
    double distance_to_target() const { return m_distance; }

    const Vec3d& pivot() const { return m_pivot; }

    double azimuth() const { return m_azimuth; }
    double zenith() const { return m_zenith; }

private:
    void set_camera_orientation();
    void normalize_azimuth_and_zenith();

private:
    Camera& m_camera;

    constexpr static double MIN_FOCAL_DISTANCE = 1e-02;

    Vec3d m_target{ Vec3d::Zero() };
    double m_distance{ 400.0 };
    Vec3d m_pivot{ Vec3d ::Zero() };
    Eigen::Quaterniond m_view_rotation{ 1.0, 0.0, 0.0, 0.0 };
    double m_azimuth{ DEFAULT_AZIMUTH };
    double m_zenith{ DEFAULT_ZENITH };
};

} // namespace Slic3r::App::Scene
