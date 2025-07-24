#pragma once

#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"
#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/App/Render/Image.hpp"

#include <memory>

namespace Slic3r::App {

struct ThumbnailStore
{
    ThumbnailStore(Biz::ProjectInteractor& project_interactor) : projects(project_interactor) {}

    void update(Domain::SelectionId project_id, const Plater::BedThumbnailTextures& thumbnails);
    void update(Domain::SelectionId project_id, Render::Image&& thumbnail_3mf);

    struct ProjectContext
    {
        Plater::BedThumbnailTextures thumbnails;
        std::unique_ptr<Render::Image> thumbnail_3mf;
    };

    using ProjectContexts = Biz::ProjectScoped<ProjectContext>;

    ProjectContexts projects;
};

} // namespace Slic3r::App
