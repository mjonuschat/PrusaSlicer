#pragma once

#include "Slic3r/App/Yoga/FlexSizer.hpp"
#include "Slic3r/App/Yoga/SplitterSizer.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/App/Yoga/MiddleSizer.hpp"

#define main_with_splitters 0

namespace Slic3r::App {

enum class ToolbarID {
    Top = 0,
    Middle,
    Bottom
};

class AbstractRenderLayout
{
public:
    AbstractRenderLayout() {};

    void render(ImVec2 size);

    void set_object_list_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_object_list_render = render_fn; }

    void set_cube_view_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_cube_view_render = render_fn; }

    void set_sidebar_bed_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_bed_render = render_fn; }

    void set_sidebar_print_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_sidebar_print_render = render_fn; }

    bool is_inited() { return m_main_sizer.is_inited(); };

    void add_toolbar_item(ToolbarID id, wchar_t icon, const std::string& tooltip, const std::string& shortcut, Yoga::Toolbar::Callbacks callbacks);
    void add_toolbar_separator(ToolbarID id, float size = GImGui->Style.WindowPadding.y);

    void show_left(int panel_id, bool show);
    void show_right(int panel_id, bool show);

private:
    void init_main_sizer();
    void init_view_cube_sizer();
    void init_toolbars_sizer();
    void layout_toolbars_sizer();
    void init_middle_sizer();

protected:
    virtual void init_left_sizer()   = 0;
    virtual void add_middle_flex_sizer();
    virtual void init_right_sizer()  = 0;

    void add_item(Yoga::FlexSizer& sizer, std::function<void(ImVec2, ImVec2)> render_item_fn, std::string item_name, ImVec2 padding = GImGui->Style.FramePadding * 4);

private:
#if main_with_splitters
    Yoga::SplitterSizer     m_main_sizer;
#else
    Yoga::FlexSizer         m_main_sizer;
#endif
    Yoga::FlexSizer         m_toolbars_sizer;

    FlexToolbar             top_toolbar;
    FlexToolbar             middle_toolbar;
    FlexToolbar             bottom_toolbar;

protected:
    Yoga::FlexSizer         left_sizer;
    Yoga::FlexSizer         view_cube_sizer;
    Yoga::FlexSizer         middle_sizer;
    Yoga::FlexSizer         right_sizer;

    std::function<void(ImVec2, ImVec2)> m_cb_object_list_render;
    std::function<void(ImVec2, ImVec2)> m_cb_cube_view_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_bed_render;
    std::function<void(ImVec2, ImVec2)> m_cb_sidebar_print_render;
};

} // namespace Slic3r::App::AbstractRenderLayout
