#pragma once
#include "Slic3r/Domain/Vectors.hpp"

namespace Slic3r::App {

class CubeView
{
public:
    CubeView() {}
    
    void init() {}

    static void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:
};

} // namespace Slic3r::App