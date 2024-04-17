#include <Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp>
#include "Slic3r/App/TestRenderModule.hpp"

int main(int argc, char** argv)
{
    Slic3r::App::Platform::SDL::SDLRenderCanvas canvas;
    Slic3r::App::TestRenderModule render_module;
    canvas.set_render_module(&render_module);

    while (!canvas.should_quit()) {
        canvas.poll_events();
        canvas.render();
    }

    return 0;
}
