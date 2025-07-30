#pragma once

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/Biz/Arrange/Settings.hpp"

namespace Slic3r::App::Yoga {
class SliderWithInput;
class ToggleButton;
class Slider;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class PivotPicker;

class ArrangeDialog final : public Yoga::GizmoDialog
{
public:
    using OnArrange = std::function<void(Biz::Arrange::Settings)>;

    ArrangeDialog(OnArrange on_arrange, const Biz::Arrange::Settings& settings);

    void set_bed_segments(const std::optional<Domain::Bed::Segments>& bed_segments);

private:
    OnArrange m_on_arrange;
    Yoga::SliderWithInput* m_offset_slider{nullptr};
    Yoga::SliderWithInput* m_bed_offset_slider{nullptr};
    Yoga::Slider* m_mode_slider{nullptr};
    Yoga::ToggleButton* m_enable_rotations_toggle{nullptr};
    PivotPicker* m_pivot_picker{nullptr};
    Yoga::Item* m_bed_segments_row{nullptr};
    Yoga::Separator* m_bed_segments_separator{nullptr};
    std::optional<Domain::Bed::Segments> m_bed_segments;

    Biz::Arrange::Settings get_settings() const;
};

} // namespace Slic3r::App::Plater
