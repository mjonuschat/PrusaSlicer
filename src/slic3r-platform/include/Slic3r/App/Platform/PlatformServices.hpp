#pragma once

#include <cassert>

#include "IRenderRequestHandler.hpp"
#include "IMainThreadDispatcher.hpp"

namespace Slic3r::App::Platform {

class PlatformServices {
public:
    static PlatformServices& instance();

    void set_services(
        IRenderRequestHandler* render_request_handler,
        IMainThreadDispatcher* main_thread_dispatcher
    );

    IRenderRequestHandler& render_request_handler() {
        assert(m_render_request_handler != nullptr);
        return *m_render_request_handler;
    }

    IMainThreadDispatcher& main_thread_dispatcher() {
        assert(m_main_thread_dispatcher != nullptr);
        return *m_main_thread_dispatcher;
    }

private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
    IMainThreadDispatcher* m_main_thread_dispatcher{nullptr};
};

}