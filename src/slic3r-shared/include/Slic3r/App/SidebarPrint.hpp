#pragma once
#include "imgui/imgui.h"

namespace Slic3r::App {

class SidebarPrint
{
public:
    SidebarPrint() {}
    
    void init() {}

    static void render(ImVec2 pos, ImVec2 size);

protected:

private:
};

} // namespace Slic3r::App