#pragma once

#include "IMainThreadDispatcher.hpp"

#include <vector>
#include <mutex>

namespace Slic3r::App::Platform {

class StdMainThreadDispatcher : public IMainThreadDispatcher
{
public:
    void dispatch_on_main_thread(Function func) override;
    void dispatch_on_main_thread_after(Function func) override;
    bool dispatch_enqueued() override;

private:
    using Functions = std::vector<Function>;
    static bool process_queue(Functions& queue, std::mutex& queue_mutex);
private:
    Functions m_queue;
    Functions m_call_after_queue;
    std::mutex m_queue_mutex;
    std::mutex m_call_after_queue_mutex;
};

}