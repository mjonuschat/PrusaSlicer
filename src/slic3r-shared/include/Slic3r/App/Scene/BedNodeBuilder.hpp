#pragma once

#include "Slic3r/App/Scene/IRenderLayerObject.hpp"

namespace Slic3r::Domain {
struct BedInstance;
} // namespace Slic3r::Domain

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {

class NodeBuilder;
struct BedNodeTag;
class ScenePresenterProjectContext;

static constexpr double BED_OFFSET_Z = -0.025;

class BedNodeBuilder
{
public:
    static void bed_node(NodeBuilder& builder, const Domain::BedInstance& instance, const BedNodeTag& tag, Render::Device& device,
        ScenePresenterProjectContext& ctx, RenderLayerId layer_id);
};

} // namespace Slic3r::App::Scene


