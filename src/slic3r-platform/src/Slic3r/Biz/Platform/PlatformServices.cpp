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

void PlatformServices::set_main_thread_dispatcher(std::unique_ptr<IMainThreadDispatcher>&& main_thread_dispatcher)
{
    ASSERT(main_thread_dispatcher, "The new main_thread_dispatcher pointer must not be nullptr!");
    ASSERT(
        m_main_thread_dispatcher == nullptr,
        "Main thread dispatched must be initialized once per application! "
        "Mutliple places take a reference to it!"
    );
    m_main_thread_dispatcher = std::move(main_thread_dispatcher);
    m_timer_queue = std::make_unique<TimerQueue>(*m_main_thread_dispatcher);
}

} // namespace Slic3r::Biz::Platform
