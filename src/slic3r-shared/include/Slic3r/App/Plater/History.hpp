#pragma once
#include "imgui/imgui.h"

namespace Slic3r::App::Plater {

class History
{
public:
    History() {}
    
    void init() {}

    static void render(ImVec2 pos, ImVec2 size);

protected:

private:
};

} // namespace Slic3r::App::Plater