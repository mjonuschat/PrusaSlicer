#pragma once

#include "Slic3r/App/Render/Image.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

#include <optional>

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Domain {
struct BedRef;
struct BedInstance;
class Project;
class ModelObject;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

struct ThumbnailRendererParams
{
    const Scene::Scene& scene;
    std::optional<Eigen::AlignedBox3d> zoom_aabb;
    Render::PixelFormat pixel_format{ Render::PixelFormat::RGBA8 };
    Render::Sizes sizes;
};

class ThumbnailRenderer
{
public:
    explicit ThumbnailRenderer(Render::Device& device) : m_device(device) {}

    [[nodiscard]] Render::Images generate_thumbnails(const ThumbnailRendererParams& params);
    [[nodiscard]] Render::Images generate_bed_thumbnails(const ThumbnailRendererParams& params, const Domain::BedRef& bed_ref,
        const Domain::BedInstance& bed_instance, Scene::CameraProjectionType camera_type);
    [[nodiscard]] Render::Images generate_object_thumbnails(const Domain::ModelObject& object, const Render::Sizes& sizes,
        Scene::CameraProjectionType camera_type, std::optional<ColorRGBA> color = std::nullopt);
    [[nodiscard]] Render::Images generate_3mf_thumbnails(const ThumbnailRendererParams& params, const Domain::Project& project,
        Scene::CameraProjectionType camera_type);
    [[nodiscard]] Render::Images generate_gcode_thumbnails(const ThumbnailRendererParams& params, const Domain::Project& project,
        const Domain::BedInstance& bed_inst, const Domain::BedRef& bed_ref, Scene::CameraProjectionType camera_type);

private:
    Render::Device& m_device;
};

} // namespace Slic3r::App::Plater