#include "Slic3r/Biz/Platform/Termination.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "spdlog/spdlog.h"

namespace Slic3r::Biz::Platform {
    void close() {
        SPDLOG_INFO("closing");
        PlatformServices::instance().main_thread_dispatcher().close();
    }
}
