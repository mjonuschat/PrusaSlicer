#include "StdMainThreadDispatcher.hpp"

namespace Slic3r::App::Platform {

void StdMainThreadDispatcher::dispatch_on_main_thread(Function func)
{
    std::scoped_lock lock(m_queue_mutex);
    m_queue.push_back(func);
}

bool StdMainThreadDispatcher::dispatch_enqueued()
{
    Functions to_process;

    {
        std::scoped_lock lock(m_queue_mutex);
        to_process.insert(
            to_process.end(),
            std::make_move_iterator(m_queue.begin()),
            std::make_move_iterator(m_queue.end())
        );
    }

    for (const auto& func : to_process)
    {
        func();
    }
    return !to_process.empty();
}

}

