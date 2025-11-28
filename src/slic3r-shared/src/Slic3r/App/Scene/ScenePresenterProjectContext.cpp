#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"

namespace Slic3r::App::Scene {

static Node& initialize_node(const std::string& debug_name, bool constant_screen_size, Scene& scene)
{
    NodeBuilder builder{scene};
    builder.set_debug_name(debug_name);
    if (constant_screen_size) {
        builder.set_screen_space_sized_modifier(SELECTION_ROOT_SCALE_MODIFIER);
    }
    Node* node{builder.build().release()};
    scene.add_child(node);
    return *node;
}

ScenePresenterProjectContext::ScenePresenterProjectContext() :
    m_scene{std::make_unique<Scene>()},
    m_selection_scene_change_session{*m_scene},
    selection_root{initialize_node("global_selection_root", true, *m_scene)},
    plain_selection_root{initialize_node("scaling_global_selection_root", false, *m_scene)}
{}

} // namespace Slic3r::App::Scene
