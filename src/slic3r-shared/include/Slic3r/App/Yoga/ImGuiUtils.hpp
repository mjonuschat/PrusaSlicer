#pragma once

#include <imgui.h>

namespace Slic3r::App::Yoga {

/**
 * @note copied from imgui internals, we need our custom styling
 */
void YGRenderArrow(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, ImU32 col, ImGuiDir dir, float scale);

} // namespace Slic3r::App::Yoga
