#pragma once

#include "Types.hpp"
#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Imgui/DoubleSlider.hpp"

namespace Slic3r::App::libvgcode {
class FdmViewer;
} // namespace Slic3r::App::libvgcode

namespace Slic3r::Biz::libpgcode {
struct PrintSettings;
} // namespace Slic3r::Biz::libpgcode

namespace Slic3r::App::Render {
class ImguiRender;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Preview {

class FdmViewerWrapper;

struct LegendCallbacks
{
    Imgui::DoubleSlider::RequestExtraFramesCallback cb_request_extra_frame{ nullptr };
    GCodeViewTypeChangedCallback                    cb_view_type_changed{ nullptr };
    ExtrusionRoleVisibilityChangedCallback          cb_extrusion_role_visibility_changed{ nullptr };
};

class Legend : public Yoga::Window {
public:
    explicit Legend(libvgcode::FdmViewer* viewer, FdmViewerWrapper* wrapper, Yoga::Item* parent = nullptr);

    LegendCallbacks& callbacks();

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

    bool settings_visible() const;
    void set_settings_visible(bool settings_visible);

private:
    libvgcode::FdmViewer* m_viewer = nullptr;
    FdmViewerWrapper* m_wrapper = nullptr;
    bool m_settings_visible = false;

    LegendCallbacks m_callback;
};

void legend(libvgcode::FdmViewer& viewer, FdmViewerWrapper& wrapper, bool settings_visible,
    const Biz::libpgcode::PrintSettings& settings, const LegendCallbacks& cbs);

void legend_coarse(libvgcode::FdmViewer& viewer, FdmViewerWrapper& wrapper);
void legend_detail(Render::ImguiRender& imgui_render, libvgcode::FdmViewer& viewer, FdmViewerWrapper& wrapper, const LegendCallbacks& cbs);

void legend_view_type_selector(libvgcode::FdmViewer& viewer, const FdmViewerWrapper& wrapper, GCodeViewTypeChangedCallback cb_view_type_changed,
    float width);

} // namespace Slic3r::App::Preview
