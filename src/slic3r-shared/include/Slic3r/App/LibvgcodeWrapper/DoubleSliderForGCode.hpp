#pragma once

#include "Slic3r/App/Imgui/DoubleSlider.hpp"

namespace Slic3r::App::LibvgcodeWrapper {

class DoubleSliderForGcode : public Imgui::DoubleSlider::Manager<unsigned int>
{
public:
    DoubleSliderForGcode() = default;
    ~DoubleSliderForGcode() = default;

    void init(int lowerPos,
              int higherPos,
              int minPos,
              int maxPos)
    {
        Manager<unsigned int>::init(lowerPos, higherPos, minPos, maxPos, "gcode_slider", true);
    }

    void set_render_as_disabled(bool value) { m_render_as_disabled = value; }
    bool is_rendering_as_disabled() const { return m_render_as_disabled; }   

    /**
     * @name Implementation of Imgui::DoubleSlider::Manager public interface
     * @{
     */
    void render(float scale_factor = 1.0f, float offset = 0.0f) override;
    /**@}*/

private:
    bool m_render_as_disabled{ false };
};

} //namespace Slic3r::App::LibvgcodeWrapper
