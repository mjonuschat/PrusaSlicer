#include "StdMainThreadDispatcher.hpp"

namespace Slic3r::App::Platform {

void StdMainThreadDispatcher::dispatch_on_main_thread(Function func)
{
    std::scoped_lock lock(m_queue_mutex);
    m_queue.push_back(func);
}

void StdMainThreadDispatcher::dispatch_on_main_thread_after(Function func)
{
    std::scoped_lock lock(m_call_after_queue_mutex);
    m_call_after_queue.push_back(func);
}

bool StdMainThreadDispatcher::process_queue(Functions& queue, std::mutex& queue_mutex)
{
    Functions to_process;

    {
        std::scoped_lock lock(queue_mutex);
        to_process.insert(
            to_process.end(),
            std::make_move_iterator(queue.begin()),
            std::make_move_iterator(queue.end())
        );
    }

    for (const auto& func : to_process)
    {
        func();
    }

    return !to_process.empty();
}

bool StdMainThreadDispatcher::dispatch_enqueued()
{
    bool main_queue = process_queue(m_queue, m_queue_mutex);
    bool after_queue = process_queue(m_call_after_queue, m_call_after_queue_mutex);
    return main_queue || after_queue;
}

}

