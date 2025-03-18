#pragma once

#include "Slic3r/App/Yoga/FlexSizer.hpp"
#include "Slic3r/App/Yoga/SplitterSizer.hpp"
#include "Slic3r/App/Yoga/MiddleSizer.hpp"

//#include <imgui/imgui.h>

namespace Slic3r::App {

class AbstractRenderLayout
{
public:
    AbstractRenderLayout() {};

    void render(ImVec2 size);

    void set_object_list_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_object_list_render = render_fn; }

    void set_sidebar_bed_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_bed_render = render_fn; }

    void set_sidebar_print_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_print_render = render_fn; }

private:
    void init_main_sizer();

protected:
    virtual void init_left_sizer()   = 0;
    virtual void init_middle_sizer() = 0;
    virtual void init_right_sizer()  = 0;

    void add_item(Yoga::FlexSizer& sizer, std::function<void(ImVec2, ImVec2)> render_item_fn, std::string item_name, ImVec2 padding = GImGui->Style.FramePadding * 4);
    void show_left(bool show);
    void show_right(bool show);

private:
    Yoga::FlexSizer         m_main_sizer;

protected:
    Yoga::FlexSizer         left_sizer;
    Yoga::MiddleSizer       middle_sizer;
    Yoga::FlexSizer         right_sizer;

    std::function<void(ImVec2, ImVec2)> m_cb_object_list_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_bed_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_print_render;
};

} // namespace Slic3r::App::AbstractRenderLayout
