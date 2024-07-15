#pragma once

#ifndef _NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

namespace Slic3r {

void init_logging();

void set_log_level(unsigned level);


}
