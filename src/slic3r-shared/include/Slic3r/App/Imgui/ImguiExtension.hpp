#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <string>
#include <sstream>

namespace Slic3r::App::Imgui {

static const ImVec4 COL_ORANGE_LIGHT = { 0.923f, 0.504f, 0.264f, 1.0f };
static const ImVec4 COL_ORANGE_DARK = { 0.67f, 0.36f, 0.19f, 1.0f };
static const ImVec4 COL_WINDOW_BACKGROUND = { 0.13f, 0.13f, 0.13f, 0.8f };
static const ImVec4 COL_GREY_LIGHT = { 0.4f, 0.4f, 0.4f, 1.0f };

static constexpr float DEFAULT_WINDOW_BG_ALPHA = 0.8f;

static const ImU32 THUMB_BG_COLOR = ImGui::ColorConvertFloat4ToU32(COL_ORANGE_LIGHT);
static const ImU32 GROOVE_BG_COLOR = ImGui::ColorConvertFloat4ToU32(COL_WINDOW_BACKGROUND);
static const ImU32 BORDER_COLOR = IM_COL32(255, 255, 255, 255);
static const ImU32 TOOLTIP_BG_COLOR = ImGui::ColorConvertFloat4ToU32(COL_GREY_LIGHT);

class UnifiedWindowStyle
{
public:
    void push();
    void pop();
};

inline void disable_background_fadeout_animation() { ImGui::GetCurrentContext()->DimBgRatio = 1.0f; }

void draw_hexagon(const ImVec2& center, float radius, ImU32 col, float start_angle = 0.0f, float rounding = 0.0f);

void tooltip(const char* label, float wrap_width = 0.0f);
void tooltip(const std::string& label, float wrap_width = 0.0f);

bool menu_item_with_icon(const char* label, const char* shortcut = nullptr, ImU32 icon_color = 0,
    bool selected = false, bool enabled = true);

bool icon_button(const wchar_t icon, const ImVec2& size = ImVec2(0.0f, 0.0f));

// this code is borrowed from https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 2)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
}

} // namespace Slic3r::App::Imgui
