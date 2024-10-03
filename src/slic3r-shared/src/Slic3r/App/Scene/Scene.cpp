#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Material.hpp"

namespace Slic3r::App::Scene {

Material resolve_material(const Node& n)
{
    std::list<const Node*> path = {&n};
    const Node* p = n.parent();
    while (p) {
        path.push_front(p);
        p = p->parent();
    }

    Material mat;

    for (const Node* component : path) {
        if (const Material* override = component->material_override())
            mat.update(*override);

    }

    return mat;
}

void Scene::render(Render::Device& device) const
{
    Node::ConstNodeList nodes;
    m_root.query([](auto n){ return n->has_render_component();}, nodes);
    std::stable_sort(nodes.begin(), nodes.end(), [](auto a, auto b) {
        return a->render_component()->layer_index() < b->render_component()->layer_index();
    });
    auto cmd_buffer = device.create_command_buffer();
    for (const auto* n : nodes) {
        auto mat_override = resolve_material(*n);
        n->render_component()->render(*n, m_camera, mat_override, *cmd_buffer);
    }
    cmd_buffer->submit();
}

}
