//
// Created by Jan Bartipan on 07.03.2024.
//

#pragma once
#include "libslic3r/TriangleMesh.hpp"
#include "slic3r3/Domain/Project.hpp"

namespace Slic3r::Biz::Interactor {

struct SceneChangeListener
{
    virtual ~SceneChangeListener() = default;
    virtual void scene_changed() = 0;
};

struct SceneSelectionChanged
{
    virtual ~SceneSelectionChanged() = default;
    virtual void scene_selection_changed() = 0;
};

class SceneInteractor
{
public:
    explicit SceneInteractor(Domain::Project *project) : m_project(project) {}

    void new_object_from_mesh(TriangleMesh &&mesh);

private:
    Domain::Project *m_project;
};

} // namespace Slic3r::Biz::Interactor
