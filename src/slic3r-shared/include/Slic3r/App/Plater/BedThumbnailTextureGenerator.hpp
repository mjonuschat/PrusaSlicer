#pragma once

#include "Slic3r/App/Plater/BedThumbnailTextureRequest.hpp"
#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"

#include <functional>
#include <deque>

namespace Slic3r::App::Scene {
class IProjectSceneProvider;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App::Plater {

typedef std::function<void(const BedThumbnailTextures&)> BedThumbnailImageGeneratorCallback;

class BedThumbnailTextureGenerator
{
public:
    BedThumbnailTextureGenerator(Render::Device& device, Biz::ProjectInteractor& project_interactor,
        Scene::IProjectSceneProvider& project_scene_provider)
        : m_device(device), m_project_interactor(project_interactor), m_project_scene_provider(project_scene_provider) {}

    void enqueue_thumbnail_requests(Domain::SelectionId project_id, const BedThumbnailTextureRequests& requests,
        BedThumbnailImageGeneratorCallback callback);

    void handle_enqueued_requests();

private:
    struct Item
    {
        Domain::SelectionId project_id;
        BedThumbnailTextureRequests requests;
        BedThumbnailImageGeneratorCallback callback;

        bool operator == (const Item& other) const {
            return project_id == other.project_id && requests == other.requests;
        }
    };

    Render::Device& m_device;
    Biz::ProjectInteractor& m_project_interactor;
    Scene::IProjectSceneProvider& m_project_scene_provider;
    std::deque<Item> m_queue;
};

} // namespace Slic3r::App::Plater
