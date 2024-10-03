#pragma once

#include "Slic3r/App/Scene/Transform.hpp"


namespace Slic3r::App::Scene {


class Camera {
public:
    Camera();

    Transform& model() { return m_model; }
    const Transform& model() const { return m_model; }
    void look_at(const Vec3f& eye, const Vec3f& center, const Vec3f& up);

    Transform& projection() { return m_projection; }
    const Transform& projection() const { return m_projection; }
    void set_perspective(float fovy, float aspect, float near_z, float far_z);

    Transform view() const
    { return m_model.inverse(); }



private:
    Transform m_model{Transform::Identity()};
    Transform m_projection{Transform::Identity()};
};

}
