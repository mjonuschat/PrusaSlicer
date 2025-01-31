#pragma once

#include <libslic3r/Color.hpp>

#include <imgui/imgui.h>

#include <array>
#include <vector>

namespace Slic3r::App::Preview {

struct ActualSpeedPlotDataItem
{
    float position{ 0.0f };
    float speed{ 0.0f };
    bool is_internal{ false };
};

struct ActualSpeedPlotData
{
    std::array<float, 2> y_range = { 0.0f, 0.0f };
    std::vector<std::pair<float, ColorRGB>> levels;
    std::vector<ActualSpeedPlotDataItem> data;
};

/** @brief Render an ImGui plot of the actual speed profile.
 *
 * @param data The data to plot
 * @param size The size of the widgets where to plot the data
 */
int plot_actual_speed_profile(const ActualSpeedPlotData& data, const ImVec2& size = { 0.0f, 0.0f });

} // namespace Slic3r::App::Preview
