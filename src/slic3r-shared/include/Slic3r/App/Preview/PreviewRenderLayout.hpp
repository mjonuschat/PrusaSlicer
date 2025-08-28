#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"

namespace Slic3r::App::Preview {

class SidebarAutoReslice;
class SidebarPreviewActionButtons;
class GCodeWindow;
class LegendWindow;
class DoubleSliderForLayers;
class DoubleSliderForGcode;

class PreviewRenderLayout : public AbstractRenderLayout
{
public:
    PreviewRenderLayout(
        std::unique_ptr<TopBar> top_bar,
        std::unique_ptr<ObjectListWindow> object_list,
        std::unique_ptr<CubeView> cube_view,
        std::unique_ptr<PopNotification::PopNotificationListView> pop_notification_list_view,
        std::unique_ptr<SidebarBed> sidebar_bed,
        std::unique_ptr<SidebarPrint> sidebar_print,
        std::unique_ptr<SidebarPreviewActionButtons> sidebar_action_buttons,
        std::unique_ptr<GCodeWindow> m_gcode_window,
        std::unique_ptr<LegendWindow> legend,
        std::unique_ptr<DoubleSliderForLayers> double_slider_layers,
        std::unique_ptr<DoubleSliderForLayers> sla_double_slider_layers,
        std::unique_ptr<DoubleSliderForGcode> double_slider_gcode,
        std::unique_ptr<SidebarAutoReslice> sidebar_auto_reslice
    );
    ~PreviewRenderLayout();

private:
    void init_left_column() override;
    void init_middle_column() override;
    void init_right_column() override;

private:
    Yoga::Passthrough<GCodeWindow> m_gcode_window;
    Yoga::Passthrough<LegendWindow> m_legend;
    Yoga::Passthrough<DoubleSliderForLayers> m_double_slider_layers;
    Yoga::Passthrough<DoubleSliderForLayers> m_sla_double_slider_layers;
    Yoga::Passthrough<DoubleSliderForGcode> m_double_slider_gcode;

    Yoga::Passthrough<SidebarAutoReslice> m_sidebar_auto_reslice;
    Yoga::Passthrough<SidebarPreviewActionButtons> m_sidebar_action_buttons;
};

} // namespace Slic3r::App::Preview
