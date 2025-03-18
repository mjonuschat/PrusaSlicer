#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <string>
#include <sstream>
#include <cstdint>

namespace Slic3r {
class ColorRGB;
class ColorRGBA;
} //  namespace Slic3r

namespace Slic3r::App::Imgui {

static constexpr float DEFAULT_WINDOW_BG_ALPHA = 0.8f;

class UnifiedWindowStyle
{
public:
    void push();
    void pop();
};

struct ScopedGroup
{
    ScopedGroup(const char* id) {
        ImGui::BeginGroup();
        ImGui::PushID(id);
    }

    ~ScopedGroup() {
        ImGui::PopID();
        ImGui::EndGroup();
    }
};

inline void disable_background_fadeout_animation() { ImGui::GetCurrentContext()->DimBgRatio = 1.0f; }

void draw_hexagon(const ImVec2& center, float radius, ImU32 col, float start_angle = 0.0f, float rounding = 0.0f);

void tooltip(const char* label, float wrap_width = 0.0f);
void tooltip(const std::string& label, float wrap_width = 0.0f);
void item_tooltip(const char* label, float wrap_width = 0.0f);
void item_tooltip(const std::string& label, float wrap_width = 0.0f);

bool menu_item_with_icon(const char* label, const char* shortcut = nullptr, ImU32 icon_color = 0,
    bool selected = false, bool enabled = true);

void icon_image(wchar_t icon, const ImVec2& size = { 0.0f, 0.0f }, bool disabled = false);

/* Use id, when the window has more than one icon_button to avoid using of the same ID generated from default label ("##btn") */
bool icon_button(wchar_t icon, const ImVec2& size = { 0.0f, 0.0f }, const std::string& id = std::string());

ImU32 to_ImU32(const ColorRGBA& color);
ImU32 to_ImU32(const ColorRGB& color, uint8_t alpha = 255);

// this code is borrowed from https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const uint8_t n = 2)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
}

} // namespace Slic3r::App::Imgui
