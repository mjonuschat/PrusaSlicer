#pragma once

#include <memory>
#include "VertexAttribDesc.hpp"

struct ImDrawData;

namespace Slic3r::App::Render {

class Device;
class CommandBuffer;
class Geometry;
class Shader;
class Texture;

class ImguiRender
{
public:
    explicit ImguiRender(Device& device);
    void new_frame();
    void render(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    void init();
    void setup_state(CommandBuffer& buffer, const ImDrawData* draw_data);
    void create_font_texture();
private:
    Device& m_device;
    VertexAttribsDesc m_vertex_format;
    std::unique_ptr<Geometry> m_geom;
    std::unique_ptr<Texture> m_font_texture;
    Shader* m_shader{nullptr};
};

} // namespace Slic3r::App::Render
