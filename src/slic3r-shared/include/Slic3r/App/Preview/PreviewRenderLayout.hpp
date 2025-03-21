#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"

namespace Slic3r::App::Preview {

class PreviewRenderLayout : public AbstractRenderLayout
{
public:
    PreviewRenderLayout() {};

    void set_legend_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_legend_render = render_fn; }

    void set_sidebar_auto_reslice_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_auto_reslice_render = render_fn; }

    void set_sidebar_after_slice_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_after_slice_render = render_fn; }

    void set_layer_slider_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_layer_slider_render = render_fn; }

    void set_gcode_slider_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_gcode_slider_render = render_fn; }

    void show_slider_sizer(bool show);
    void show_gcode_sizer(bool show);

private:
    void init_left_sizer() override;
    void add_middle_flex_sizer() override;
    void init_right_sizer() override;

private:
    Yoga::FlexSizer middle_flex_sizer;
    Yoga::FlexSizer middle_left_flex_sizer;

    std::function<void(ImVec2, ImVec2)> m_cb_legend_render;

    std::function<void(ImVec2, ImVec2)> m_cb_layer_slider_render;
    std::function<void(ImVec2, ImVec2)> m_cb_gcode_slider_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_auto_reslice_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_after_slice_render;
};

} // namespace Slic3r::App::PreviewRenderLayout
