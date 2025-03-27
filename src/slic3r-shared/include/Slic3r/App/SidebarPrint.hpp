#pragma once
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::App {

class SidebarPrint
{
public:
    SidebarPrint() {}
    
    void init() {}

    static void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:
};

} // namespace Slic3r::App
