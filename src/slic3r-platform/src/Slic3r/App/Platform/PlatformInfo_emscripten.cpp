#include "PlatformInfo.hpp"

#include <sys/utsname.h>

namespace Slic3r::App::Platform {

PlatformInfo::PlatformInfo()
    : m_os_version(Semver::invalid())
    , m_platform(Slic3r::platform())
    , m_platform_flavor(Slic3r::platform_flavor())
{}


}

