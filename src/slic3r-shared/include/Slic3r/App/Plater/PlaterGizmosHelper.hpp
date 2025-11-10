#pragma once

namespace Slic3r::App::Scene {
class Node;
} // namespace Slic3r::App::Scene

namespace Slic3r::Biz::Scene {
struct ObjectSelection;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

/**
 * @brief Returns true if the given node can be added to the given object selection.
 * @param node The node to check.
 * @param selection The object selection candidate to host the node.
 * @return true if the given node can be added to the given object selection, false otherwise.
 */
bool can_be_added_to_object_selection(const Scene::Node& node, const Biz::Scene::ObjectSelection& selection);

} // namespace Slic3r::App::Plater
