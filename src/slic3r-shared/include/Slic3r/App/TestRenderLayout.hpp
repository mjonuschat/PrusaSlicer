#pragma once

#include "Slic3r/App/Yoga/FlexSizer.hpp"
#include "Slic3r/App/Yoga/SplitterSizer.hpp"
#include "Slic3r/App/Yoga/MiddleSizer.hpp"

#define flex_with_splitters 1

namespace Slic3r::App {

class TestRenderLayout
{
public:
    TestRenderLayout() {};

    void render(ImVec2 size);

protected:

private:
    void init_main_sizer();
    void init_left_sizer();
    void init_middle_sizer();
    void init_right_sizer();

#if flex_with_splitters
    Yoga::SplitterSizer     m_main_sizer;
#else
    Yoga::YogaFlexSizer m_main_sizer;
#endif
    Yoga::SplitterSizer     m_left_sizer;
    Yoga::MiddleSizer       m_middle_sizer;
    Yoga::SplitterSizer     m_right_sizer;
};

} // namespace Slic3r::App::TestRenderLayout
