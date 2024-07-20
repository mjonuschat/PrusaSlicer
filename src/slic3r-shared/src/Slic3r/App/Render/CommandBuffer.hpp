#pragma once

namespace Slic3r::App::Render {

class CommandBuffer
{
public:
    void set_viewport();
    void set_scissor();
    void bind_geometry();
    void draw();
private:

};

}

