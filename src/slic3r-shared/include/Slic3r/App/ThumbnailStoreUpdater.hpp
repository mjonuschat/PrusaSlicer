#pragma once

#include "Slic3r/App/Plater/IBedVisuallyChangedListener.hpp"
#include "Slic3r/App/ThumbnailStore.hpp"
#include "Slic3r/Biz/ThumbnailImageProvider.hpp"
#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App {

typedef std::function<void(const Plater::BedThumbnailTextures&)> ThumbnailUpdateCallback;

class ThumbnailStoreUpdater : public Plater::IBedVisuallyChangedListener
{
public:
    ThumbnailStoreUpdater(
        Biz::ThumbnailImageProvider& thumbnail_image_provider,
        std::shared_ptr<App::ThumbnailStore> thumbnail_store
    ) :
        m_thumbnail_image_provider(thumbnail_image_provider),
        m_thumbnail_store(thumbnail_store)
    {}

    /**
     * @name Implementation of IBedVisuallyChangedListener public interface
     * @{
     */
    void on_bed_changed(Domain::SelectionId project_id, const Domain::BedRefs& bed_refs) override;
    /**@}*/

    void update(Render::Device& device, ThumbnailUpdateCallback callback = nullptr);

private:
    Biz::ThumbnailImageProvider& m_thumbnail_image_provider;
    std::shared_ptr<App::ThumbnailStore> m_thumbnail_store;
    std::future<Biz::ThumbnailImageResults> m_thumbnail_results;
};

} // namespace Slic3r::App
