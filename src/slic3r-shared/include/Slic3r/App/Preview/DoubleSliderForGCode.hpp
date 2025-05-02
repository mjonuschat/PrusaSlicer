#pragma once

#include "Slic3r/App/Imgui/DoubleSlider.hpp"

namespace Slic3r::App::Preview {

class DoubleSliderForGcode : public Imgui::DoubleSlider::Manager<unsigned int>
{
public:
    explicit DoubleSliderForGcode(Slic3r::App::Yoga::Item* parent = nullptr)
        : Imgui::DoubleSlider::Manager<unsigned int>(std::string("gcode_slider"), parent)
    {}

    void init(int lowerPos, int higherPos, int minPos, int maxPos)
    {
        Manager<unsigned int>::init(lowerPos, higherPos, minPos, maxPos, "gcode_slider", true);
    }

    void set_render_as_disabled(bool value) { m_render_as_disabled = value; }
    bool is_rendering_as_disabled() const { return m_render_as_disabled; }

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

private:
    bool m_render_as_disabled{false};
};

} //namespace Slic3r::App::Preview
