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

private:

    void    init_middle_top_sizer();
    void    init_middle_bottom_sizer();
    void    init_middle_bottom_left_toolbar();
    void    init_middle_bottom_right_toolbar();

private:

    FlexSizer       m_top_sizer;
    FlexSizer       m_bottom_sizer;

public:

    FlexToolbar     top_left_toolbar;
    FlexToolbar     top_right_toolbar;

    FlexToolbar     bl_toolbar;
    FlexToolbar     br_toolbar;
                    
    FlexToolbar     top_middle_toolbar;
    FlexToolbar     left_middle_toolbar;
};

} // namespace Slic3r::App::Yoga::MiddleSizer
