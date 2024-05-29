#pragma once

#include <functional>
#include <vector>

namespace Slic3r::App::Platform {

class IMainThreadDispatcher
{
public:
    using Function = std::function<void()>;
    virtual ~IMainThreadDispatcher() = default;

    virtual void dispatch_on_main_thread(Function func) = 0;
    virtual void dispatch_on_main_thread_after(Function func) = 0;
    virtual bool dispatch_enqueued() = 0;
};

}