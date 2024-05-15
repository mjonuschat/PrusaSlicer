#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp>
#include "Slic3r/App/TestRenderModule.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <functional>
#define EMSCRIPTEN_MAINLOOP_BEGIN       MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END         ; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif

#ifdef __EMSCRIPTEN__
#define WAIT_FOR_EVENT 0
#else
#define WAIT_FOR_EVENT 1
#endif

std::function<void()> main_loop_impl;
void main_loop()
{
    if (main_loop_impl)
        main_loop_impl();
}


int main(int argc, char** argv)
{
    Slic3r::App::Platform::SDL::SDLRenderCanvas canvas;

    Slic3r::App::Platform::PlatformServices::instance().set_services(&canvas, &canvas);

    Slic3r::App::TestRenderModule render_module;
    canvas.set_render_module(&render_module);

#ifdef __EMSCRIPTEN__
    main_loop_impl = [&]()
#else
    while (!canvas.should_quit())
#endif
    {
#if WAIT_FOR_EVENT
        canvas.wait_for_events();
#else
        canvas.poll_events();
#endif
        canvas.render();
    };
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
#endif

    return 0;
}
