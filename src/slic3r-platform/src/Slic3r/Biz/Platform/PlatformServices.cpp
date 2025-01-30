#include <Slic3r/Biz/Platform/PlatformServices.hpp>

namespace Slic3r::Biz::Platform {

PlatformServices& PlatformServices::instance()
{
    static PlatformServices instance;
    return instance;
}

void PlatformServices::set_render_request_handler(
    IRenderRequestHandler* render_request_handler
)
{
    m_render_request_handler = render_request_handler;
}

void PlatformServices::set_main_thread_dispatcher(IMainThreadDispatcher* main_thread_dispatcher)
{
    m_main_thread_dispatcher = main_thread_dispatcher;
}

} // namespace Slic3r::Biz::Platform
