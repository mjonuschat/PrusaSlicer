#include "Slic3r/App/Scene/GeometryManager.hpp"

namespace Slic3r::App::Scene {

Render::Geometry* GeometryManager::get_or_create(const std::string& name, std::function<std::unique_ptr<Render::Geometry>()> builder)
{
    auto it = m_geometries.find(name);
    if (it != m_geometries.end())
        return it->second.get();
    auto ret = m_geometries.emplace(name, builder());
    return ret.first->second.get();
}


}
