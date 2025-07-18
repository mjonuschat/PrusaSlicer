#pragma once

#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/App/Plater/Measure.hpp"
#include "Slic3r/App/Scene/Ray.hpp"

#include <memory>

namespace Slic3r::Domain {
class TriangleMesh;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {
class Node;
} // Slic3r::App::Scene

namespace Slic3r::App::Plater::Measure {

struct VolumeCacheItem
{
    Domain::ElementRef ref;
    const Domain::TriangleMesh* mesh{ nullptr };
    Domain::Transform3d world_trafo;
    int face_offset{ 0 };
};

struct InstanceCacheItem
{
    Domain::ElementRef ref;
    std::unique_ptr<Measuring> measuring;
    Scene::Node* node{ nullptr };
    bool modified{ false };
};

struct SceneSelectionCache
{
    Domain::ElementRefs volume_ids;
    std::vector<VolumeCacheItem> volumes;
    std::vector<InstanceCacheItem> instances;

    void reset()
    {
        volume_ids.clear();
        volumes.clear();
        instances.clear();
    }
};

enum class MeasureGizmoElementType : int8_t
{
    Undefined,
    CurrentFeature,
    FirstSelectedFeature,
    SecondSelectedFeature,
    Dimensioning,
};

struct MeasureGizmoNodeTag
{
    const MeasureGizmoElementType type{ MeasureGizmoElementType::Undefined };
};

struct FeatureItem
{
    Domain::ElementRef ref;
    SurfaceFeature feature;
    std::optional<SurfaceFeature> parent;
};

struct FeatureCache
{
    std::optional<FeatureItem> current;
    std::array<std::optional<FeatureItem>, 2> selected;

    void reset() {
        current.reset();
        selected[0].reset();
        selected[1].reset();
    }
};

struct FeatureDetectionData
{
    const VolumeCacheItem* hovered_volume;
    const InstanceCacheItem* hovered_instance;
    const Scene::Node* node;
    Scene::Ray pick_ray;
    double hit_coord;
};

} // namespace Slic3r::App::Plater::Measure
