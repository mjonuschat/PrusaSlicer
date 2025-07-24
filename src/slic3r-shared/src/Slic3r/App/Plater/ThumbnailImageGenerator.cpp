#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/App/Scene/IProjectSceneProvider.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Plater {

void ThumbnailImageGenerator::enqueue_thumbnail_requests(
    const Biz::ThumbnailImageRequests& requests,
    std::promise<Biz::ThumbnailImageResults>&& promise
)
{
    DEBUG_ASSERT(!requests.empty());

    for (const auto& request : requests) {
        Item item{.requests = requests, .promise = std::move(promise)};

        if (std::find(m_queue.begin(), m_queue.end(), item) == m_queue.end())
            m_queue.emplace_back(std::move(item));
    }

    Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
}

void ThumbnailImageGenerator::handle_enqueued_requests()
{
    while (!m_queue.empty()) {
        Item& item = m_queue.front();

        Biz::ThumbnailImageResults results;
        for (const auto& request : item.requests) {
            const Domain::Project& project = m_workbench.project(request.params.project_id);
            Scene::Scene& scene = m_scene_provider.project_scene(request.params.project_id);

            Biz::ThumbnailImageResult& result = results.emplace_back(Biz::ThumbnailImageResult());
            result.type                       = request.type;
            result.project_id                 = request.params.project_id;
            result.bed_instance_id            = request.params.bed_instance_id;

            switch (request.type) {
            // gallery
            case Biz::ThumbnailType::Object: {
                break;
            }
            // objects list
            case Biz::ThumbnailType::SceneBed: {
                Plater::ThumbnailRendererParams params{
                    .scene        = scene,
                    .pixel_format = request.params.pixel_format,
                    .sizes        = request.params.sizes
                };
                result.images = m_renderer.generate_bed_thumbnails(
                    params,
                    project,
                    request.params.bed_instance_id,
                    Scene::CameraProjectionType::Orthographic
                );
                break;
            }
            // gcode
            case Biz::ThumbnailType::SlicingBed: {
                Plater::ThumbnailRendererParams params{
                    .scene        = scene,
                    .pixel_format = request.params.pixel_format,
                    .sizes        = request.params.sizes
                };
                result.images = m_renderer.generate_gcode_thumbnails(
                    params,
                    project,
                    request.params.bed_instance_id,
                    Scene::CameraProjectionType::Orthographic
                );
                break;
            }
            // 3mf
            case Biz::ThumbnailType::Scene: {
                Plater::ThumbnailRendererParams params{
                    .scene        = scene,
                    .pixel_format = request.params.pixel_format,
                    .sizes        = request.params.sizes
                };
                result.images = m_renderer.generate_3mf_thumbnails(
                    params,
                    project,
                    Scene::CameraProjectionType::Orthographic
                );
                break;

                break;
            }
            }
        }

        item.promise.set_value(std::move(results));
        m_queue.pop_front();
    }
}

} // namespace Slic3r::App::Plater
