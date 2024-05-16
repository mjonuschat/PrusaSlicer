#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp>
#include "Slic3r/App/TestRenderModule.hpp"

#include "Windows.h"

//int main(int argc, char** argv)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    Slic3r::App::Platform::SDL::SDLRenderCanvas canvas;

    Slic3r::App::Platform::PlatformServices::instance().set_services(&canvas, &canvas);

    Slic3r::App::TestRenderModule render_module;
    canvas.set_render_module(&render_module);

    while (!canvas.should_quit()) {
        canvas.poll_events();
        canvas.render();
    }

    return 0;
}
