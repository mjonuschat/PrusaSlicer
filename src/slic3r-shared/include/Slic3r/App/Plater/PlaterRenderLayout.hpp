#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"

namespace Slic3r::App::Plater {

class PlaterRenderLayout : public AbstractRenderLayout
{
public:
    PlaterRenderLayout() {};

    void set_history_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_history_render = render_fn; }

    void set_sidebar_slice_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_slice_render = render_fn; }

private:
    void init_left_sizer() override;
    void init_right_sizer() override;

    std::function<void(ImVec2, ImVec2)> m_cb_history_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_slice_render;
};

} // namespace Slic3r::App::PlaterRenderLayout
