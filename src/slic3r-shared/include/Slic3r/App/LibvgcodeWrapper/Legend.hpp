#pragma once

#include "Slic3r/App/LibvgcodeWrapper/Types.hpp"
#include "Slic3r/App/Imgui/DoubleSlider.hpp"

namespace Slic3r::App::libvgcode {
class Viewer;
} // namespace Slic3r::App::libvgcode

namespace Slic3r::Biz::libpgcode {
struct PrintSettings;
} // namespace Slic3r::Biz::libpgcode

namespace Slic3r::App::LibvgcodeWrapper {

class WrapperImpl;

struct LegendCallbacks
{
    Imgui::DoubleSlider::RequestExtraFramesCallback cb_request_extra_frame{ nullptr };
    GCodeViewTypeChangedCallback                    cb_view_type_changed{ nullptr };
    ExtrusionRoleVisibilityChangedCallback          cb_extrusion_role_visibility_changed{ nullptr };
};

void legend(libvgcode::Viewer& viewer, WrapperImpl& wrapper, bool settings_visible,
    const Biz::libpgcode::PrintSettings& settings, const LegendCallbacks& cbs);

void legend_coarse(libvgcode::Viewer& viewer, WrapperImpl& wrapper);

void legend_view_type_selector(libvgcode::Viewer& viewer, const WrapperImpl& wrapper, GCodeViewTypeChangedCallback cb_view_type_changed,
    float width);

} // namespace Slic3r::App::LibvgcodeWrapper
