#pragma once

#include "Slic3r/App/Yoga/FlexSizer.hpp"
#include "Slic3r/App/Yoga/SplitterSizer.hpp"
#include "Slic3r/App/Yoga/MiddleSizer.hpp"

#define flex_with_splitters 0

namespace Slic3r::App {

class TestRenderLayout
{
public:
    TestRenderLayout() {};

    void render(ImVec2 size);

    void set_object_list_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
        m_cb_object_list_render = render_fn;
    }

protected:

private:
    void init_main_sizer();
    void init_left_sizer();
    void init_middle_sizer();
    void init_right_sizer();

#if flex_with_splitters
    Yoga::SplitterSizer     m_main_sizer;
#else
    Yoga::FlexSizer m_main_sizer;
#endif
    Yoga::SplitterSizer     m_left_sizer;
    Yoga::MiddleSizer       m_middle_sizer;
    Yoga::SplitterSizer     m_right_sizer;

    std::function<void(ImVec2, ImVec2)> m_cb_object_list_render;
};

} // namespace Slic3r::App::TestRenderLayout
