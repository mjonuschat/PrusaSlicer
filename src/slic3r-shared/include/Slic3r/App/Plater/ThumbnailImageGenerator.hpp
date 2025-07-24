#pragma once

#include "Slic3r/Biz/IThumbnailImageGenerator.hpp"
#include "Slic3r/App/Plater/ThumbnailRenderer.hpp"

#include <deque>

namespace Slic3r::Domain {
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {
class IProjectSceneProvider;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Plater {

class ThumbnailImageGenerator : public Biz::IThumbnailImageGenerator
{
public:
    ThumbnailImageGenerator(
        const Domain::Workbench& workbench,
        Render::Device& device,
        Scene::IProjectSceneProvider& scene_provider
    ) :
        m_workbench(workbench),
        m_device(device),
        m_scene_provider(scene_provider),
        m_renderer(device)
    {}

    void enqueue_thumbnail_requests(
        const Biz::ThumbnailImageRequests& requests,
        std::promise<Biz::ThumbnailImageResults>&& promise
    ) override;
    void handle_enqueued_requests() override;

private:
    struct Item
    {
        Biz::ThumbnailImageRequests requests;
        std::promise<Biz::ThumbnailImageResults> promise;

        bool operator==(const Item& other) const
        {
            return requests == other.requests;
        }
    };

    const Domain::Workbench& m_workbench;
    Render::Device& m_device;
    Scene::IProjectSceneProvider& m_scene_provider;
    ThumbnailRenderer m_renderer;
    std::deque<Item> m_queue;
};

} // namespace Slic3r::App::Plater
