#pragma once

#include "Slic3r/App/Scene/Camera.hpp"

namespace Slic3r::App::Scene {

class CameraTrackballController
{
public:
    explicit CameraTrackballController(Camera& camera) : m_camera(camera) {}

    void set_focal_point(const Vec3f& pos)
    {
        m_cam_focal = pos;
        update_camera();
    }

    void set_focal_distance(float value)
    {
        m_cam_focal_dist = std::max(MIN_FOCAL_DISTANCE, value);
        update_camera();
    }

    void set_azimuth(float value)
    {
        m_azimuth = value;
        update_camera();
    }

    void set_zenith(float value);

    void add_azimuth(float value)
    {
        m_azimuth += value;
        update_camera();
    }

    void add_zenith(float value)
    {
        m_zenith += value;
        clamp_zenith();
        update_camera();
    }

    void add_azimuth_and_zenith(float azimuth, float zenith)
    {
        m_azimuth += azimuth;
        m_zenith += zenith;
        clamp_zenith();
        update_camera();
    }

    const Vec3f& cam_focal() const { return m_cam_focal; }
    float cam_focal_dist() const { return m_cam_focal_dist; }
    float azimuth() const { return m_azimuth; }
    float zenith() const { return m_zenith; }

private:
    void update_camera();
    void clamp_zenith();
private:
    Camera& m_camera;

    constexpr static float MIN_FOCAL_DISTANCE = 1e-02;
    constexpr static float MIN_ZENITH = 1e-02;

    Vec3f m_cam_focal{0,0,0};
    float m_cam_focal_dist{30};
    float m_azimuth{M_PI/2};
    float m_zenith{M_PI/2};

};

}
