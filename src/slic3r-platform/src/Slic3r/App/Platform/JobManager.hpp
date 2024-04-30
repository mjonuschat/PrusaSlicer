#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <functional>

namespace Slic3r::App::Platform {

class JobManager
{
public:
    using Function = std::function<void()>;
    void enqueue(Function func);

private:
    std::vector<std::thread> m_threads;
    std::mutex m_queue_mutex;
};

}

