#pragma once

#include "Slic3r/App/imgui/DoubleSlider.hpp"

#include <memory>

namespace Slic3r::App::LibvgcodeWrapper {

struct WrapperSettings
{
    bool slider_layers_editable{ false };
    bool slider_layers_show_ruler{ false };
    bool slider_layers_show_ruler_bg{ false };
    bool slider_layers_show_estimated_times{ false };
    bool settings_in_legend_visible{ false };
    bool gcodewindow_visible{ true };
};

class WrapperImpl;

class Wrapper
{
public:
    Wrapper();
    Wrapper(Wrapper&&) = delete;
    Wrapper(const Wrapper&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;
    Wrapper& operator=(const Wrapper&) = delete;
    ~Wrapper();

    bool init(const WrapperSettings& settings);
    void shutdown();

    void reset();

private:
    std::unique_ptr<WrapperImpl> m_impl;
};

} // namespace Slic3r::App::LibvgcodeWrapper
