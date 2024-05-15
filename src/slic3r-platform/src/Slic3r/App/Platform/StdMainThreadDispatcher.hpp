#pragma once

#include "IMainThreadDispatcher.hpp"

#include <vector>
#include <mutex>

namespace Slic3r::App::Platform {

class StdMainThreadDispatcher : public IMainThreadDispatcher
{
public:
    void dispatch_on_main_thread(Function func) override;
    bool dispatch_enqueued() override;

private:
    using Functions = std::vector<Function>;
    Functions m_queue;
    std::mutex m_queue_mutex;
};

}