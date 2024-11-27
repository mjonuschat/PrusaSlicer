#pragma once

#include "Slic3r/App/Scene/Camera.hpp"

namespace Slic3r::App::Scene {

class CameraTrackballController
{
public:
    explicit CameraTrackballController(Camera& camera) : m_camera(camera) { update_camera(); }

    void set_focal_point(const Vec3d& pos)
    {
        m_cam_focal = pos;
        update_camera();
    }

    void set_focal_distance(double value)
    {
        m_cam_focal_dist = std::max(MIN_FOCAL_DISTANCE, value);
        update_camera();
    }

    void set_azimuth(double value)
    {
        m_azimuth = value;
        normalize_azimuth_and_zenith();
        update_camera();
    }

    void set_zenith(double value);

    void add_azimuth(double value)
    {
        m_azimuth += value;
        normalize_azimuth_and_zenith();
        update_camera();
    }

    void add_zenith(double value)
    {
        m_zenith += value;
        normalize_azimuth_and_zenith();
        update_camera();
    }

    void add_azimuth_and_zenith(double azimuth, double zenith)
    {
        m_azimuth += azimuth;
        m_zenith += zenith;
        normalize_azimuth_and_zenith();
        update_camera();
    }

    const Vec3d& cam_focal() const { return m_cam_focal; }
    double cam_focal_dist() const { return m_cam_focal_dist; }
    double azimuth() const { return m_azimuth; }
    double zenith() const { return m_zenith; }

private:
    void update_camera();
    void normalize_azimuth_and_zenith();
private:
    Camera& m_camera;

    constexpr static double MIN_FOCAL_DISTANCE = 1e-02;

    Vec3d m_cam_focal{0,0,0};
    double m_cam_focal_dist{200};
    double m_azimuth{-M_PI_2};
    double m_zenith{-M_PI_2};

};

}
