#pragma once

#include "Slic3r/App/Yoga/FlexSizer.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"

namespace Slic3r::App::Yoga {

class MiddleSizer : public FlexSizer
{
public:
    MiddleSizer() {};

    void    render(ImVec2 win_size = ImVec2(), ImVec2 win_pos = ImVec2(-1.f, -1.f)) override;
    void    initialize();
    void    layout() override;
    void    set_bottom_middle_sizer_render_fn(std::function<void(ImVec2, ImVec2)> render_fn) {
            m_cb_bottom_middle_sizer_render = render_fn; }

private:

    void    init_top_sizer();
    void    init_bottom_sizer();
    void    init_bottom_middle_sizer();

private:

    FlexSizer       m_top_sizer;
    FlexSizer       m_bottom_sizer;
    std::function<void(ImVec2, ImVec2)> m_cb_bottom_middle_sizer_render;

public:

    FlexToolbar     top_left_toolbar;
    FlexToolbar     top_right_toolbar;
    FlexToolbar     top_middle_toolbar;

    FlexToolbar     bottom_left_toolbar;
    FlexToolbar     bottom_right_toolbar;
                    
    FlexToolbar     left_middle_toolbar;
    FlexSizer       bottom_middle_sizer;
};

} // namespace Slic3r::App::Yoga::MiddleSizer
