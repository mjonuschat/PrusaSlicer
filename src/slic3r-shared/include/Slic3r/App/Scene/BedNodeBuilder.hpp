#pragma once

#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"

namespace Slic3r::Domain {
struct BedInstance;
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {

class NodeBuilder;
struct BedNodeTag;

class BedNodeBuilder
{
public:
    static void bed_node(NodeBuilder& builder, const Domain::BedInstance& instance, const BedNodeTag& tag, Render::Device& device,
        ScenePresenterProjectContext& ctx, RenderLayerId layer_id);
};

} // namespace Slic3r::App::Scene


