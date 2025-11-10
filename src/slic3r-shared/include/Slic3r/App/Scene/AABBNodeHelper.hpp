#pragma once

#include "Slic3r/App/Scene/IRenderLayerObject.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

#include <string>
#include <optional>

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {

class NodeBuilder;
class ScenePresenterProjectContext;
class Node;

/**
  * @brief Node tag for aabb
  */
struct AABBNodeTag
{
    uint8_t corner_id{ 0 };
};

void build_aabb_node(NodeBuilder& builder, ScenePresenterProjectContext& ctx, Render::Device& device, const std::string& debug_name,
    RenderLayerId layer_id, const Domain::ColorRGB& color = Domain::ColorRGB::WHITE());

void update_aabb_node(Node& node, const Eigen::AlignedBox3d& aabb, double edge_coverage_percent = 1.0,
    std::optional<Domain::ColorRGB> color = std::nullopt);

void update_aabb_node(Node& node, const Domain::BoundingBox3d& aabb, double edge_coverage_percent = 1.0,
    std::optional<Domain::ColorRGB> color = std::nullopt);

} // namespace Slic3r::App::Scene
