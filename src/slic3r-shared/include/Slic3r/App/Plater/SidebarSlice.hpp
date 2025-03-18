#pragma once
#include "imgui/imgui.h"

namespace Slic3r::App::Plater {

class SidebarSlice
{
public:
    SidebarSlice() {}
    
    void init() {}

    static void render(ImVec2 pos, ImVec2 size);

protected:

private:
};

} // namespace Slic3r::App::Plater