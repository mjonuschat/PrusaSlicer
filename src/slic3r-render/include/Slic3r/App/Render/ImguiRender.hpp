#pragma once

#include "VertexAttribDesc.hpp"
#include "Slic3r/App/Render/ImguiFontHelper.hpp"

#include <memory>

struct ImDrawData;

namespace Slic3r::App::Render {

class Device;
class CommandBuffer;
class Geometry;
class Shader;

class ImguiRender
{
public:
    explicit ImguiRender(Device& device);
    ~ImguiRender();

    const std::string& language() const { return m_font_helper.language(); }
    float font_size() const { return m_font_helper.font_size(); }

    void set_font(const std::optional<std::string>& language = std::nullopt, const std::optional<float>& font_size = std::nullopt, const std::optional<float>& font_global_scale = std::nullopt);

    void new_frame();
    void render(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    void init();
    void setup_state(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    Device& m_device;
    VertexAttribsDesc m_vertex_format;
    std::unique_ptr<Geometry> m_geom;
    ImguiFontHelper m_font_helper;
    Shader* m_shader{nullptr};
};

} // namespace Slic3r::App::Render
