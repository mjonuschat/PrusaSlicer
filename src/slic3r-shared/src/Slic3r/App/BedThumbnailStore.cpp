#include "Slic3r/App/BedThumbnailStore.hpp"

namespace Slic3r::App {

void BedThumbnailStore::update(const Plater::BedThumbnailTextures& thumbnails)
{
    projects.project(thumbnails.front().project_id).thumbnails = thumbnails;
}

} // namespace Slic3r::App
