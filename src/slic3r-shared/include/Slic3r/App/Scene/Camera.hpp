#pragma once

#include <memory>

#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Render/Types.hpp"
#include "Slic3r/Biz/ListenerList.hpp"

namespace Slic3r::App::Scene {

class Camera;

/**
 * @brief Interface for camera projection computation.
 */
class AbstractCameraProjection
{
public:
    AbstractCameraProjection() = default;
    AbstractCameraProjection(double z_near, double z_far)
        : m_z_near(z_near), m_z_far(z_far)
    {}
    virtual ~AbstractCameraProjection() = default;

    /**
     * @brief Compute projection matrix for given @p viewport
     * @param viewport Camera viewport position and size
     * @return Projection matrix
     */
    virtual Transform projection(const Render::Rect& viewport) const = 0;

    /**
     * @brief Compute screen space size scale.
     *
     * This scale is applied by ScreenSpaceSizedTransformModifier to keep screen space size constant
     * for a given node.
     *
     * @param cam A camera object
     * @param cam_object_dist A distance from camera to node.
     * @return Scale to be applied
     *
     * @note
     * Typically:
     * - for perspective projection this should be @f(\frac{dist}{2 \cdot tan \frac{fov_y}{2} }@f)
     * (where @f(dist@f) is camera-node disance, and @f(fov_y@f) is Field of view in vertical axis),
     * - for ortho projection this should be @f(\frac{2}{r - l}@f) (where @f(l@f) and @f(r@f) are
     * left resp. right parameters of ortho projection).
     * .
     */
    virtual double constant_screen_space_size_scale(const Camera& cam, double cam_object_dist) const = 0;

    /**
     * @brief Get the distance of the near plane from the camera eye
     * @return The distance of the near plane from the camera eye
     */
    double z_near() const { return m_z_near; }
    /**
     * @brief Set the distance of the near plane from the camera eye
     * @param val Get the distance of the near plane from the camera eye
     */
    void set_z_near(double val) { m_z_near = val; }
    /**
     * @brief Get the distance of the far plane from the camera eye
     * @return The distance of the far plane from the camera eye
     */
    double z_far() const { return m_z_far; }
    /**
     * @brief Set the distance of the far plane from the camera eye
     * @param val Get the distance of the far plane from the camera eye
     */
    void set_z_far(double val) { m_z_far = val; }

protected:
    double m_z_near{ 10.0f };
    double m_z_far{ 1000.f };
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
    void look_at(const Vec3d& eye, const Vec3d& center, const Vec3d& up);

    Transform& projection() { return m_projection; }
    const Transform& projection() const { return m_projection; }

    Transform view() const
    { return m_model.inverse(); }

    Vec3d position() const { return m_model.block<3, 1>(0, 3); }

    Vec3d forward() const { return -view().block<1, 3>(2, 0); }
    Vec3d right() const { return view().block<1, 3>(0, 0); }
    Vec3d up() const { return view().block<1, 3>(1, 0); }

    void set_viewport(const Render::Rect& viewport);
    const Render::Rect viewport() const { return m_viewport; }

    Ray ray_at(double screen_x, double screen_y) const;
    Vec3d unproject(const Vec3d& win_pos) const;

    const AbstractCameraProjection& cam_projection() const { return *m_projection_getter; }
    void set_cam_projection(AbstractCameraProjection* projection_getter)
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
    std::unique_ptr<AbstractCameraProjection> m_projection_getter;
    CameraUpdateListeners m_update_listeners;
};

class PerspectiveCameraProjection : public AbstractCameraProjection
{
public:
    PerspectiveCameraProjection() = default;
    PerspectiveCameraProjection(double fovy, double z_near, double z_far)
        : AbstractCameraProjection(z_near, z_far), m_fovy(fovy)
    {
    }
    Transform projection(const Render::Rect& viewport) const override;
    double constant_screen_space_size_scale(const Camera& cam, double cam_object_dist) const override;

    double fovy() const { return m_fovy; }
    void set_fovy(double val) { m_fovy = val; }

private:
    double m_fovy{90};
};


}
