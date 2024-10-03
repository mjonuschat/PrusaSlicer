#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Slic3r/App/Render/Geometry.hpp"

namespace Slic3r::App::Scene {

class GeometryManager {
public:
    Render::Geometry* get_or_create(const std::string& name, std::function<std::unique_ptr<Render::Geometry>()> builder);

    bool release(const std::string& name) { return m_geometries.erase(name) > 0; }
    void release_all() { m_geometries.clear(); }

private:
    using GeometryMap = std::unordered_map<std::string, std::unique_ptr<Render::Geometry>>;

    GeometryMap m_geometries;
};

}
