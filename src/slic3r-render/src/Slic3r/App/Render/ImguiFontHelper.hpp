#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <imgui/imgui.h>

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <optional>
#include <cmath>

namespace Slic3r::App::Render {

class Device;
class Texture;

struct ImguiLanguageData
{
    std::string language;
    // Chinese, Japanese, Korean
    float font_size{ 18.0f };
    // language prefix, ranges, whether it needs CLK font
    std::vector<std::tuple<std::string, const ImWchar*, bool>> lang_glyphs_info;
    const ImWchar* glyph_ranges{ nullptr };
    std::map<wchar_t, int> custom_glyph_rects_ids;
};

using ImguiFonts = std::map<ImguiFontType, ImFont*>;

class ImguiFontHelper
{
public:
    explicit ImguiFontHelper(Device& device);

    void set_font(const std::optional<std::string>& language = std::nullopt, 
                  const std::optional<float>& font_size = std::nullopt, 
                  const std::optional<float>& font_global_scale = std::nullopt);

    const std::string& language() const { return m_language_data.language; }

    float font_size() const { return m_language_data.font_size; }
    float font_scale() const { return font_size() / 16.0f; }

    int icon_size() const { return lround(16 * font_scale()); } // default size of icon is 16 px
    int icon_medium_size() const { return int(1.25f * icon_size()); }
    int icon_large_size() const { return 2 * icon_size(); }
    int icon_extra_large_size() const { return 4 * icon_size(); }
    int icon_toolbar_size() const { return 2.5f * icon_size(); }
    float icon_advance() const { return 3.0f * font_scale(); }

    ImFont* font(Render::ImguiFontType type);

private:
    void create_font_texture();

private:
    Device& m_device;
    ImguiLanguageData m_language_data;
    Texture* m_font_texture{ nullptr };
    ImguiFonts m_fonts;
};

} // namespace Slic3r::App::Render
