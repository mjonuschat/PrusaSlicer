#pragma once
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::App::Preview {

class SidebarAutoReslice
{
public:
    SidebarAutoReslice() {}
    
    void init() {}

    static void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:
};

} // namespace Slic3r::App::Preview
