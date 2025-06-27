#include "Slic3r/Biz/AppInstance/AppInstanceUtils.hpp"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <atomic>
#include <random>
#endif

namespace Slic3r::Biz::AppInstance {
unsigned get_current_pid()
{
#ifdef WIN32
    return GetCurrentProcessId();
#elif __APPLE__
    return ::getpid();
#else
    // On flatpak getpid() might return same number for each concurent instances.
    static std::atomic<unsigned> instance_uuid{0};
    if (instance_uuid == 0) {
        unsigned generated_value;
        {
            // Use a thread-local random engine
            thread_local std::random_device rd;
            thread_local std::mt19937 generator(rd());
            std::uniform_int_distribution<unsigned> distribution;
            generated_value = distribution(generator);
        }
        unsigned expected = 0;
        // Atomically initialize the instance_uuid if it has not been set
        instance_uuid.compare_exchange_strong(expected, generated_value);
    }
    return instance_uuid.load();
#endif
}
} // namespace Slic3r::Biz::AppInstance
