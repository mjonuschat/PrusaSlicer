#pragma once

#include <memory>

#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Render/Types.hpp"
#include "Slic3r/Biz/ListenerList.hpp"

namespace Slic3r::App::Scene {

class Camera;

class ICameraProjectionGetter
{
public:
    virtual ~ICameraProjectionGetter() = default;
    virtual Transform projection(const Render::Rect& viewport) const = 0;
    virtual float constant_screen_space_size_scale(const Camera& cam, float cam_object_dist) const = 0;
};

class ICameraUpdateListener
{
public:
    virtual ~ICameraUpdateListener() = default;

    virtual void camera_updated(const Camera& cam) = 0;
};

class Camera {
public:
    Camera();

    Transform& model() { return m_model; }
    const Transform& model() const { return m_model; }
    void look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up);

    Transform& projection() { return m_projection; }
    const Transform& projection() const { return m_projection; }

    Transform view() const
    { return m_model.inverse(); }

    void set_viewport(const Render::Rect& viewport);
    const Render::Rect viewport() const { return m_viewport; }

    Ray ray_at(float screen_x, float screen_y) const;
    Vec3f unproject(const Vec3f& win_pos) const;

    const ICameraProjectionGetter& projection_getter() const { return *m_projection_getter; }
    void set_projection_getter(ICameraProjectionGetter* projection_getter)
    {
        m_projection_getter.reset(projection_getter);
    }

    void add_update_listener(ICameraUpdateListener* update_listener)
    {
         m_update_listeners.add(update_listener);
    }

    void remove_update_listener(ICameraUpdateListener* update_listener)
    {
        m_update_listeners.remove(update_listener);
    }

private:
    using CameraUpdateListeners = Biz::ListenerList<ICameraUpdateListener>;

    Transform m_model{Transform::Identity()};
    Transform m_projection{Transform::Identity()};
    Render::Rect m_viewport;
    std::unique_ptr<ICameraProjectionGetter> m_projection_getter;
    CameraUpdateListeners m_update_listeners;
};

class PerspectiveCameraProjectionGetter : public ICameraProjectionGetter
{
public:
    PerspectiveCameraProjectionGetter() = default;
    PerspectiveCameraProjectionGetter(float fovy, float z_near, float z_far)
        : m_fovy(fovy), m_z_near(z_near), m_z_far(z_far)
    {}
    Transform projection(const Render::Rect& viewport) const override;
    float constant_screen_space_size_scale(const Camera& cam, float cam_object_dist) const override;

    float fovy() const { return m_fovy; }
    void set_fovy(float val) { m_fovy = val; }

    float z_near() const { return m_z_near; }
    void set_z_near(float val) { m_z_near = val; }
    float z_far() const { return m_z_far; }
    void set_z_far(float val) { m_z_far= val; }

private:
    float m_fovy{90};
    float m_z_near{0.01f};
    float m_z_far{100.f};
};


}
