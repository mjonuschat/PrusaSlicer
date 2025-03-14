#pragma once

#include "VertexAttribDesc.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <imgui/imgui.h>

#include <memory>
#include <string>
#include <optional>

struct ImDrawData;

namespace Slic3r::App::Render {

class Device;
class CommandBuffer;
class Geometry;
class Shader;
class ImguiFontHelper;

class ImguiRender
{
public:
    explicit ImguiRender(Device& device);
    ~ImguiRender();

    const std::string& language() const;
    float font_size() const;

    void set_font(const std::optional<std::string>& language = std::nullopt, const std::optional<float>& font_size = std::nullopt,
        const std::optional<float>& font_global_scale = std::nullopt);

    ImFont* font(Render::ImguiFontType type);

    void new_frame();
    void render(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    void init();
    void setup_state(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    Device& m_device;
    VertexAttribsDesc m_vertex_format;
    std::unique_ptr<Geometry> m_geom;
    std::unique_ptr<ImguiFontHelper> m_font_helper;
    Shader* m_shader{nullptr};
};

} // namespace Slic3r::App::Render
