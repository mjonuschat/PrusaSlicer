#include "Slic3r/App/Plater/BedThumbnailTextureGenerator.hpp"
#include "Slic3r/App/Plater/ScopedBedThumbnailSceneCustomizer.hpp"
#include "Slic3r/App/Plater/ThumbnailRenderer.hpp"
#include "Slic3r/App/Scene/IProjectSceneProvider.hpp"
#include "Slic3r/App/Scene/ThumbnailHelper.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"

#include <fmt/core.h>

namespace Slic3r::App::Plater {

void BedThumbnailTextureGenerator::enqueue_thumbnail_requests(Domain::SelectionId project_id, const BedThumbnailTextureRequests& requests,
    BedThumbnailImageGeneratorCallback callback)
{
    for (const auto& request : requests) {
        Item item{
            .project_id = project_id,
            .requests = requests,
            .callback = callback
        };

        if (std::find(m_queue.begin(), m_queue.end(), item) == m_queue.end())
            m_queue.push_back(item);
    }
}

void BedThumbnailTextureGenerator::handle_enqueued_requests()
{
    while (!m_queue.empty()) {
        auto& [project_id, requests, callback] = m_queue.front();
        const Domain::Project& proj = m_project_interactor.selected_project();
        Scene::Scene& scene = m_project_scene_provider.project_scene(project_id);
        BedThumbnailTextures results;

        for (const auto& request : requests) {
            const Domain::ConfigContainer* cc = proj.find_config_container(request.bed_ref.config_container_id);
            const Domain::BedInstance& bed_instance = cc->find_bed_instance(request.bed_ref.instance_id);

            ThumbnailRendererParams params{
                .scene = scene,
                .pixel_format = request.pixel_format,
                .sizes = request.sizes
            };

            ThumbnailRenderer renderer(m_device);
            Render::Images images = renderer.generate_bed_thumbnails(params, request.bed_ref, bed_instance,
                Scene::CameraProjectionType::Perspective);

            for (const auto& image : images) {
                std::string name = fmt::format("thumbnail_bed_{}_{}_{}x{}",
                    request.bed_ref.config_container_id, request.bed_ref.instance_id, image.width(), image.height());
                BedThumbnailTexture& tex = results.emplace_back();
                tex.project_id = request.project_id;
                tex.bed_ref = request.bed_ref;
                tex.thumbnail = m_device.context().texture_manager().get_or_create_dynamic(name, image.format(), image.width(),
                    image.height());
                tex.thumbnail->set_data(image.format(), 0, image.width(), image.height(), image.data());
            }

#if ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG
            std::string name = fmt::format("thumbnail_bed_{}_{}", request.bed_ref.config_container_id, request.bed_ref.instance_id);
            std::string path_prefix = fmt::format("C:/test/{}", name);
            Scene::export_to_png_file(images, path_prefix);
#endif // ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG

        }
        callback(results);
        m_queue.pop_front();
    }
}

} // namespace Slic3r::App::Plater
