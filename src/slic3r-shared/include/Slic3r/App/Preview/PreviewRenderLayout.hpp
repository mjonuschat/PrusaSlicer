#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"


namespace Slic3r::App::Preview {

class SidebarAutoReslice;
class SidebarPreviewActionButtons;
class GCodeWindow;
class Legend;
class DoubleSliderForLayers;
class DoubleSliderForGcode;

class PreviewRenderLayout : public AbstractRenderLayout
{
public:
    PreviewRenderLayout(
        ObjectList* object_list,
        CubeView* cube_view,
        SidebarBed* sidebar_bed,
        SidebarPrint* sidebar_print,
        SidebarPreviewActionButtons* sidebar_action_buttons,
        GCodeWindow* m_gcode_window,
        Legend* legend,
        DoubleSliderForLayers* double_slider_layers,
        DoubleSliderForGcode* double_slider_gcode,
        SidebarAutoReslice* sidebar_auto_reslice
    );

private:
    void init_left_column() override;
    void init_middle_column() override;
    void init_right_column() override;

private:
    GCodeWindow* m_gcode_window = nullptr;
    Legend* m_legend = nullptr;
    DoubleSliderForLayers* m_double_slider_layers = nullptr;
    DoubleSliderForGcode* m_double_slider_gcode = nullptr;
    SidebarAutoReslice* m_sidebar_auto_reslice = nullptr;
    SidebarPreviewActionButtons* m_sidebar_action_buttons = nullptr;
};

} // namespace Slic3r::App::Preview
