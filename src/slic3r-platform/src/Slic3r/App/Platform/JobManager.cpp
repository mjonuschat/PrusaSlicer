#include "JobManager.hpp"

namespace Slic3r::App::Platform {

void JobManager::enqueue(Function func)
{
    std::scoped_lock lock(m_queue_mutex);
    m_threads.emplace_back([func, this]() {
        func();
        {
            std::scoped_lock lock(m_queue_mutex);

        }
    });
}

}
