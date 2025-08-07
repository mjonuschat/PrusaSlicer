#include "Slic3r/App/ThumbnailStoreUpdater.hpp"
#include "Slic3r/App/ObjectListWindow.hpp"
#include "Slic3r/App/ThumbnailStore.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"

#include <fmt/core.h>

namespace Slic3r::App {

void ThumbnailStoreUpdater::on_bed_changed(Domain::SelectionId project_id, const Domain::BedRefs& bed_refs)
{
    static const Domain::Size SIZE = {256, 256};

    if (m_thumbnail_results.valid())
        return;

    Biz::ThumbnailImageRequests requests;
    requests.reserve(bed_refs.size() + 1);

    // thumbnails for object list
    for (const auto& bed_ref : bed_refs) {
        Biz::ThumbnailImageRequest& request = requests.emplace_back(Biz::ThumbnailImageRequest());
        request.type                        = Biz::ThumbnailType::SceneBed;
        request.params.project_id           = project_id;
        request.params.bed_instance_id      = bed_ref.instance_id;
        request.params.pixel_format         = Domain::PixelFormat::RGBA8;
        request.params.sizes                = {SIZE};
    }

    // thumbnails for 3mf
    Biz::ThumbnailImageRequest& request = requests.emplace_back(Biz::ThumbnailImageRequest());
    request.type                        = Biz::ThumbnailType::Scene;
    request.params.project_id           = project_id;
    request.params.bed_instance_id      = 0;
    request.params.pixel_format         = Domain::PixelFormat::RGBA8;
    request.params.sizes                = {SIZE};

    m_thumbnail_results = m_thumbnail_image_provider.generate_thumbnails(requests);
}

void ThumbnailStoreUpdater::update(Render::Device& device, ThumbnailUpdateCallback callback)
{
    if (m_thumbnail_results.valid()) {
        std::future_status status = m_thumbnail_results.wait_for(std::chrono::milliseconds(5));
        if (status == std::future_status::ready) {
            Biz::ThumbnailImageResults thumbnail_results = m_thumbnail_results.get();
            if (!thumbnail_results.empty()) {
                // all results have the same project_id, so we can use the first one
                Domain::SelectionId project_id = thumbnail_results.front().project_id;
                Plater::BedThumbnailTextures textures;
                for (auto& t : thumbnail_results) {
                    switch (t.type) {
                    // thumbnails for object list
                    case Biz::ThumbnailType::SceneBed: {
                        if (!t.images.empty()) {
                            for (const auto& image : t.images) {
                                std::string name = fmt::format(
                                    "thumbnail_bed_{}_{}_{}x{}",
                                    t.project_id,
                                    t.bed_instance_id,
                                    image.width(),
                                    image.height()
                                );
                                Plater::BedThumbnailTexture& tex = textures.emplace_back(
                                    Plater::BedThumbnailTexture()
                                );
                                tex.project_id      = t.project_id;
                                tex.bed_instance_id = t.bed_instance_id;
                                tex.thumbnail = device.context().texture_manager().get_or_create_dynamic(
                                    name,
                                    image.format(),
                                    image.width(),
                                    image.height()
                                );
                                tex.thumbnail->set_data(
                                    image.format(),
                                    0,
                                    image.width(),
                                    image.height(),
                                    image.pixels.data()
                                );
                            }
                        }
                        break;
                    }
                    // thumbnail for 3mf
                    case Biz::ThumbnailType::Scene: {
                        if (!t.images.empty())
                            m_thumbnail_store->update(project_id, std::move(t.images.front()));
                        break;
                    }
                    default: {
                        break;
                    }
                    }
                }

                m_thumbnail_store->update(project_id, textures);
                if (callback != nullptr)
                    callback(textures);
            }
        }
    }
}

} // namespace Slic3r::App
