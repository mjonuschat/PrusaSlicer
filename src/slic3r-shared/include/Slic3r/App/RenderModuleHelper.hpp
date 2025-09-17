#pragma once

#define ENABLED_DEBUG_OUTLINE 0

#if ENABLED_DEBUG_OUTLINE
namespace Slic3r::App::Scene {
class Node;
} // namespace Slic3r::App::Scene
#endif // ENABLED_DEBUG_OUTLINE

namespace Slic3r::App {

#if ENABLED_DEBUG_OUTLINE
void imgui_scenegraph_node_info(const Scene::Node& node);
#endif // ENABLED_DEBUG_OUTLINE

} // namespace Slic3r::App