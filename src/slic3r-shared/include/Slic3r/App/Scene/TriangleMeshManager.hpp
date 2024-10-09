#pragma once
#include "Slic3r/App/Render/ResourceManager.hpp"

#include "libslic3r/AABBMesh.hpp"

namespace Slic3r::App::Scene
{

struct TriangleMesh
{
    indexed_triangle_set triangles;
    std::unique_ptr<AABBMesh> aabb_mesh;

    TriangleMesh() = default;
    explicit TriangleMesh(indexed_triangle_set&& triangles)
        : triangles(std::move(triangles)), aabb_mesh(std::make_unique<AABBMesh>(this->triangles))
    {}
};

template <typename K>
using TriangleMeshManager = Render::ResourceManager<K, TriangleMesh>;

}
