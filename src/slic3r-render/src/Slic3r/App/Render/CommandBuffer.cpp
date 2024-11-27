#include "Slic3r/App/Render/CommandBuffer.hpp"

#include "Slic3r/App/Render/Geometry.hpp"

#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLTypes.hpp"
#include "Slic3r/App/Render/GL/GLCommandBufferInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// NOLINT_BEGIN(*-convert-member-functions-to-static)

namespace Slic3r::App::Render {

void CommandBuffer::bind_material(const Material& material)
{
    bind_shader(*material.shader());

    for (const auto [slot, texture] : material.textures())
        bind_texture(slot, *texture);

    for (const auto& [name, value] : material.uniforms())
        set_uniform(*material.shader(), name.c_str(), value);
}

void CommandBuffer::unbind_material(const Material& material)
{
    for (const auto [slot, texture] : material.textures())
        unbind_texture(slot, *texture);
}


void CommandBuffer::draw(const DrawCommand& cmd)
{
    draw(cmd.primitive, cmd.offset, cmd.count);
}

void CommandBuffer::draw(const DrawCommands::const_iterator first, const DrawCommands::const_iterator last)
{
    std::for_each(first, last, [this](const auto& cmd) { draw(cmd); });
}

void CommandBuffer::bind_and_draw(const Geometry& g, const Material& material_override)
{
    const auto& cmds = g.draw_commands();
    if (!DEBUG_ASSERT_VAL(!cmds.empty()))
        return;

    const auto* top_shader = material_override.shader();
    bool top_shader_bound = false;

    for (const auto& cmd : cmds) {
        Material material = cmd.material;
        material.update(material_override);
        // if command overrides shader
        if (const auto* shader = cmd.material.shader()) {
            bind_shader(*shader);
            bind_geometry(g, *shader);
            top_shader_bound = false;
        } else if (!top_shader_bound) {
            bind_shader(*DEBUG_ASSERT_VAL(top_shader));
            bind_geometry(g, *top_shader);
            top_shader_bound = true;
        }
        bind_material(material);
        draw(cmd);
        unbind_material(material);
    }
}

}
