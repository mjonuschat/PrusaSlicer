#include "Slic3r/App/Plater/BedThumbnailUpdater.hpp"
#include "Slic3r/App/Plater/BedThumbnailTextureGenerator.hpp"
#include "Slic3r/App/ObjectListWindow.hpp"
#include "Slic3r/App/BedThumbnailStore.hpp"

#include <fmt/core.h>

namespace Slic3r::App::Plater {

void BedThumbnailUpdater::on_bed_changed(Domain::SelectionId project_id, const Domain::BedRefs& bed_refs)
{
    static const Render::Size SIZE = { 256, 256 };

    BedThumbnailTextureRequests requests;
    requests.reserve(bed_refs.size());
    for (const auto& bed_ref : bed_refs) {
        BedThumbnailTextureRequest& request = requests.emplace_back();
        request.project_id = project_id;
        request.bed_ref = bed_ref;
        request.pixel_format = Render::PixelFormat::RGBA8;
        request.sizes = { SIZE };
    }
    
    m_thumbnail_generator.enqueue_thumbnail_requests(project_id, requests, [&](const BedThumbnailTextures& results) {
        if (!results.empty()) {
            m_object_list.set_bed_instance_icons(results);
            m_store.update(results);
        }
    });
}

} // namespace Slic3r::App::Plater