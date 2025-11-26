#pragma once

#include "Slic3r/App/Yoga/GizmoWindow.hpp"
#include "Slic3r/Biz/Arrange/Settings.hpp"

namespace Slic3r::App::Yoga {
class SliderWithInput;
class ToggleButton;
class Slider;
class SegmentedControl;
class Separator;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class PivotPicker;

enum class ArrangeTaskStatus
{
    Idle,
    Running
};

class ArrangeDialog final : public Yoga::GizmoWindow
{
public:
    using OnArrange      = std::function<void(Biz::Arrange::Settings)>;
    using OnCancel       = std::function<void()>;
    using OnModeSelected = std::function<void(Biz::Arrange::Mode)>;

    ArrangeDialog(
        OnArrange on_arrange,
        OnCancel on_cancel,
        OnModeSelected on_mode_selected,
        const Biz::Arrange::Settings& settings
    );

    void set_bed_segments(const std::optional<Domain::Bed::Segments>& bed_segments);

    void update_status(const ArrangeTaskStatus status);

    Biz::Arrange::Mode get_arrange_mode() const;
    void set_arrange_mode(Biz::Arrange::Mode mode);

private:
    OnArrange m_on_arrange;
    OnCancel m_on_cancel;
    Yoga::SegmentedControl* m_mode{nullptr};
    Yoga::SliderWithInput* m_offset_slider{nullptr};
    Yoga::SliderWithInput* m_bed_offset_slider{nullptr};
    Yoga::Slider* m_mode_slider{nullptr};
    Yoga::ToggleButton* m_enable_rotations_toggle{nullptr};
    PivotPicker* m_pivot_picker{nullptr};
    Yoga::Item* m_bed_segments_row{nullptr};
    Yoga::Separator* m_bed_segments_separator{nullptr};
    Yoga::LayoutButton* m_arrange_button{nullptr};
    std::optional<Domain::Bed::Segments> m_bed_segments;

    Biz::Arrange::Settings get_settings() const;
};

} // namespace Slic3r::App::Plater
