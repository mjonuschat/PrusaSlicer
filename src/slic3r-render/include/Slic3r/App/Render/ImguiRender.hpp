#pragma once

#include "VertexAttribDesc.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <imgui/imgui.h>

#include <memory>
#include <string>
#include <optional>
#include <list>

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
    /**
     * @note do not forget to register texture with use_texture when rendering them
     */
    TexturePtr icon_texture(Icon icon, int max_size);

    void new_frame();
    void render(CommandBuffer& buffer, const ImDrawData* draw_data);

    /**
     * @brief use_texture - registers TexturePtr which is shared_ptr
     * to ImGuiRender, who will hold this pointer until end of the render cycle.
     * @note all textures rendered by Yoga::Item should be registered here
     */
    void use_texture(TexturePtr texture);
private:
    void init();
    void setup_state(CommandBuffer& buffer, const ImDrawData* draw_data);
private:
    Device& m_device;
    VertexAttribsDesc m_vertex_format;
    std::unique_ptr<Geometry> m_geom;
    std::unique_ptr<ImguiFontHelper> m_font_helper;
    Shader* m_shader{nullptr};
    std::list<TexturePtr> m_in_use_textures;
};

} // namespace Slic3r::App::Render
