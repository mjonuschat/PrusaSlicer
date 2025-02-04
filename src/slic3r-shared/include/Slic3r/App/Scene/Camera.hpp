#pragma once

#include <memory>

#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Render/Types.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"

namespace Slic3r::App::Scene {

class Camera;

enum class CameraProjectionType : uint8_t
{
    Perspective,
    Orthographic
};

struct CameraProjectionParameters
{
    static double orthographic_zoom_from_perspective(double perspective_zoom) { return 1 / (REF_Z * std::tan((REF_FOVY * M_PI) / (2 * 180 *  perspective_zoom))); }
    static double perspective_zoom_from_orthographic(double ortho_zoom) { return (REF_FOVY * M_PI) / (180 * 2 * std::atan(1 / (REF_Z * ortho_zoom))); }

    static constexpr double REF_FOVY = 90.0;

    // static constexpr double Z_NEAR = 10;
    // static constexpr double Z_FAR = 1000.0;
    static constexpr double REF_Z = 300;

    static constexpr double PERSPECTIVE_MIN_ZOOM = 0.6;
    static constexpr double PERSPECTIVE_MAX_ZOOM = 100;

    static double orthographic_min_zoom() { return orthographic_zoom_from_perspective(PERSPECTIVE_MIN_ZOOM); }
    static double orthographic_max_zoom() { return orthographic_zoom_from_perspective(PERSPECTIVE_MAX_ZOOM); }
};

/**
 * @brief Interface for camera projection computation.
 */
class AbstractCameraProjection
{
public:
    explicit AbstractCameraProjection(CameraProjectionType type)
        : m_type(type)
    {}
    AbstractCameraProjection(CameraProjectionType type, double z_near, double z_far)
        : m_type(type), m_z_near(z_near), m_z_far(z_far)
    {}
    virtual ~AbstractCameraProjection() = default;

    /**
     * @brief Compute projection matrix for given @p viewport
     * @param viewport Camera viewport position and size
     * @param zoom Zoom value to apply
     * @return Projection matrix
     *
     * @note
     * Zoom:
     * - for perspective projection the zoom modifies the vertical field of view by applying a scaling factor equal to 1/zoom
     * - for orthographic projection the zoom modifies the viewport by applying a scaling factor equal to 1/zoom
     */
    virtual Transform projection(const Render::Rect& viewport, double zoom) const = 0;

    virtual double min_zoom() const = 0;
    virtual double max_zoom() const = 0;

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
     * @brief Get the type of projection
     * @return The type of projection
     */
    CameraProjectionType type() const { return m_type; }
    /**
     * @brief Get the distance of the near plane from the camera eye
     * @return The distance of the near plane from the camera eye
     */
    double z_near() const { return m_z_near; }
    /**
     * @brief Set the distance of the near plane from the camera eye with the given value
     * @param val The new value for the distance of the near plane from the camera eye
     */
    void set_z_near(double val) { m_z_near = val; }
    /**
     * @brief Get the distance of the far plane from the camera eye
     * @return The distance of the far plane from the camera eye
     */
    double z_far() const { return m_z_far; }
    /**
     * @brief Set the distance of the far plane from the camera eye with the given value
     * @param val The new value for the distance of the far plane from the camera eye
     */
    void set_z_far(double val) { m_z_far = val; }

protected:
    CameraProjectionType m_type;
    double m_z_near{ 10. };
    double m_z_far{ 1000. };
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

    void switch_projection_type();

    Transform view() const
    { return m_model.inverse(); }

    Vec3d position() const { return m_model.block<3, 1>(0, 3); }

    Vec3d forward() const { return -view().block<1, 3>(2, 0); }
    Vec3d right() const { return view().block<1, 3>(0, 0); }
    Vec3d up() const { return view().block<1, 3>(1, 0); }

    void set_viewport(const Render::Rect& viewport);
    const Render::Rect& viewport() const { return m_viewport; }

    void set_zoom(double value);
    void update_zoom(double value) { set_zoom(m_zoom / (1.0 - std::max(std::min(value, 4.0), -4.0) * 0.1)); }
    double zoom() const { return m_zoom; }

    Ray ray_at(double screen_x, double screen_y) const;
    Vec3d unproject(const Vec3d& win_pos) const;

    const AbstractCameraProjection& cam_projection() const { return *m_projection_getter; }

    void add_update_listener(ICameraUpdateListener* update_listener)
    {
         m_update_listeners.add(update_listener);
    }

    void remove_update_listener(ICameraUpdateListener* update_listener)
    {
        m_update_listeners.remove(update_listener);
    }

private:
    void update_projection();

private:
    using CameraUpdateListeners = Biz::ListenerList<ICameraUpdateListener>;

    Transform m_model{Transform::Identity()};
    Transform m_projection{Transform::Identity()};
    Render::Rect m_viewport;
    double m_zoom{ 1. };
    std::unique_ptr<AbstractCameraProjection> m_projection_getter;
    CameraUpdateListeners m_update_listeners;
};

class PerspectiveCameraProjection : public AbstractCameraProjection
{
public:
    PerspectiveCameraProjection()
        : AbstractCameraProjection(CameraProjectionType::Perspective)
    {}

    Transform projection(const Render::Rect& viewport, double zoom) const override;
    double constant_screen_space_size_scale(const Camera& cam, double cam_object_dist) const override;

    double fovy() const { return m_fovy; }

    double min_zoom() const override { return CameraProjectionParameters::PERSPECTIVE_MIN_ZOOM; }
    double max_zoom() const override { return CameraProjectionParameters::PERSPECTIVE_MAX_ZOOM; }

private:
    const double m_fovy{CameraProjectionParameters::REF_FOVY};
};

class OrthographicCameraProjection : public AbstractCameraProjection
{
public:
    OrthographicCameraProjection()
        : AbstractCameraProjection(CameraProjectionType::Orthographic)
    {}
    OrthographicCameraProjection(double z_near, double z_far)
        : AbstractCameraProjection(CameraProjectionType::Orthographic, z_near, z_far)
    {}

    Transform projection(const Render::Rect& viewport, double zoom) const override;
    double constant_screen_space_size_scale(const Camera& cam, double cam_object_dist) const override;

    double min_zoom() const override { return CameraProjectionParameters::orthographic_min_zoom(); }
    double max_zoom() const override { return CameraProjectionParameters::orthographic_max_zoom(); }

};

}
