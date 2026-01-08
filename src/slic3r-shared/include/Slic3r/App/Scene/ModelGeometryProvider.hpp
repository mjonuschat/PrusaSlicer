#pragma once

#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/AuxiliaryElementId.hpp"

namespace Slic3r::App::Scene {

struct ModelGeometryProvider
{
    using GeometryManager = Render::GeometryManager<AuxiliaryElementId>;
    using TriangleMeshManager = TriangleMeshManager<AuxiliaryElementId>;

    GeometryManager geometry_manager;
    TriangleMeshManager triangle_mesh_manager;
};

struct ISharedModelGeometryProvider
{
    virtual ~ISharedModelGeometryProvider() = default;
    virtual std::shared_ptr<ModelGeometryProvider> shared_model_geometry_provider() = 0;
};

} // namespace Slic3r::App::Scene