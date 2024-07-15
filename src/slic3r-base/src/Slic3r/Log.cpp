#include "Slic3r/Log.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <spdlog/sinks/msvc_sink.h>
#endif


namespace Slic3r {

static spdlog::level::level_enum s_current_log_level = spdlog::level::info;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
static std::shared_ptr<spdlog::sinks::msvc_sink_mt> msvc_sink;
#endif

static spdlog::level::level_enum log_level_to_spdlog(unsigned int level)
{
    switch (level) {
    // Report fatal errors only.
    case 0:
        return spdlog::level::critical;
    // Report fatal errors and errors.
    case 1:
        return spdlog::level::err;
    // Report fatal errors, errors and warnings.
    case 2:
        return spdlog::level::warn;
    // Report all errors, warnings and infos.
    case 3:
        return spdlog::level::info;
    // Report all errors, warnings, infos and debugging.
    case 4:
        return spdlog::level::debug;
    // Report everything including fine level tracing information.
    default:
        return spdlog::level::trace;
    }
}


void init_logging()
{
    // TODO: add optional logging to file here

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->sinks().push_back(msvc_sink); });
#endif
    spdlog::set_level(s_current_log_level);
}

void set_log_level(unsigned level) 
{ 
    auto lvl = log_level_to_spdlog(level);
    s_current_log_level = lvl;
    spdlog::set_level(lvl); 
}


}
