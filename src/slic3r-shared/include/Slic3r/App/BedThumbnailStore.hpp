#pragma once

#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"
#include "Slic3r/Biz/ProjectScoped.hpp"

namespace Slic3r::App {

struct BedThumbnailStore
{
    BedThumbnailStore(Biz::ProjectInteractor& project_interactor) : projects(project_interactor) {}

    void update(const Plater::BedThumbnailTextures& thumbnails);

    struct ProjectContext
    {
        Plater::BedThumbnailTextures thumbnails;
    };
    using ProjectContexts = Biz::ProjectScoped<ProjectContext>;

    ProjectContexts projects;
};

} // namespace Slic3r::App
