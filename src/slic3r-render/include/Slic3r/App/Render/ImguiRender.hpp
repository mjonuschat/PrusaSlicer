#pragma once

#include "VertexAttribDesc.hpp"

#include <imgui/imgui.h>

#include <map>
#include <string>
#include <memory>
#include <optional>

struct ImDrawData;

namespace Slic3r::App::Render {

class Device;
class CommandBuffer;
class Geometry;
class Shader;
class Texture;

struct ImguiLanguageHelper
{
    std::string language;
    // Chinese, Japanese, Korean
    float font_size{ 18.0f };
    // language prefix, ranges, whether it needs CLK font
    std::vector<std::tuple<std::string, const ImWchar*, bool>> lang_glyphs_info;
    const ImWchar* glyph_ranges{ nullptr };
    std::map<wchar_t, int> custom_glyph_rects_ids;
};

class ImguiRender
{
public:
    explicit ImguiRender(Device& device);

    const std::string& language() const { return m_language_helper.language; }
    float font_size() const { return m_language_helper.font_size; }

    void set_font(const std::optional<std::string>& language = std::nullopt, const std::optional<float>& font_size = std::nullopt);

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
    ImguiLanguageHelper m_language_helper;
    Texture* m_font_texture{ nullptr };
    Shader* m_shader{nullptr};
};

} // namespace Slic3r::App::Render
