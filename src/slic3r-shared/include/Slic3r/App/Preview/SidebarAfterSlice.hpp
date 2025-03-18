#pragma once
#include "imgui/imgui.h"

namespace Slic3r::App::Preview {

class SidebarAfterSlice
{
public:
    SidebarAfterSlice() {}
    
    void init() {}

    static void render(ImVec2 pos, ImVec2 size);

protected:

private:
};

} // namespace Slic3r::App::Preview