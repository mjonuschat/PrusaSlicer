#pragma once
#include "Slic3r/Domain/Vectors.hpp"

namespace Slic3r::App::Plater {

class History
{
public:
    History() {}
    
    void init() {}

    static void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:
};

} // namespace Slic3r::App::Plater