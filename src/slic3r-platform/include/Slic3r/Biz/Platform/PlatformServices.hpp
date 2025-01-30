#pragma once

#include <libassert/assert.hpp>

#include "IRenderRequestHandler.hpp"
#include "IMainThreadDispatcher.hpp"

namespace Slic3r::Biz::Platform {

class PlatformServices
{
public:
    static PlatformServices& instance();

    void set_render_request_handler(IRenderRequestHandler* render_request_handler);

    void set_main_thread_dispatcher(IMainThreadDispatcher* main_thread_dispatcher);

    IRenderRequestHandler& render_request_handler()
    {
        ASSERT(m_render_request_handler != nullptr);
        return *m_render_request_handler;
    }

    IMainThreadDispatcher& main_thread_dispatcher()
    {
        ASSERT(m_main_thread_dispatcher != nullptr);
        return *m_main_thread_dispatcher;
    }

private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
    IMainThreadDispatcher* m_main_thread_dispatcher{nullptr};
};

} // namespace Slic3r::Biz::Platform
