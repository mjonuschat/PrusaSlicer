#pragma once

#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"
#include "Slic3r/App/LibvgcodeWrapper/DoubleSliderForGCode.hpp"
#include "Slic3r/App/LibvgcodeWrapper/DoubleSliderForLayers.hpp"
#include "Slic3r/App/LibvgcodeWrapper/GCodeWindow.hpp"
#include "Slic3r/App/LibvgcodeWrapper/ActualSpeedPlotter.hpp"

namespace Slic3r::App::LibvgcodeWrapper {

struct LegendParams
{
    bool visible{ true };
    bool enabled{ true };
    bool settings_visible{ false };
    bool is_shown() const { return enabled && visible; }
};

class WrapperImpl
{
public:
    WrapperImpl() = default;
    WrapperImpl(WrapperImpl&&) = delete;
    WrapperImpl(const WrapperImpl&) = delete;
    WrapperImpl& operator=(WrapperImpl&&) = delete;
    WrapperImpl& operator=(const WrapperImpl&) = delete;
    ~WrapperImpl() = default;

    bool init(const WrapperSettings& settings);
    void shutdown();

    void reset();

private:
    WrapperSettings m_settings;

    DoubleSliderForGcode m_slider_gcode;
    DoubleSliderForLayers m_slider_layers;
    GCodeWindowData m_gcode_window_data;
    ActualSpeedPlotData m_actual_speed_plot_data;
    LegendParams m_legend_params;
};

} // namespace Slic3r::App::LibvgcodeWrapper
