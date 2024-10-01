//
#pragma once
#include "libslic3r/TriangleMesh.hpp"
#include "Slic3r/Domain/Project.hpp"

namespace Slic3r::Biz::Scene {


class SceneInteractor
{
public:
    explicit SceneInteractor(Domain::Project *project) : m_project(project) {}

    void new_object_from_mesh(TriangleMesh &&mesh);

private:
    Domain::Project *m_project;
};

} // namespace Slic3r::Biz::Interactor
