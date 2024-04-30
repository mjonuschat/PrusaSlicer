#include "PlatformServices.hpp"

namespace Slic3r::App::Platform {

PlatformServices& PlatformServices::instance()
{
    static PlatformServices instance;
    return instance;
}

void PlatformServices::set_services(
    IRenderRequestHandler* render_request_handler,
    IMainThreadDispatcher* main_thread_dispatcher
) {
    m_render_request_handler = render_request_handler;
    m_main_thread_dispatcher = main_thread_dispatcher;
}


}
