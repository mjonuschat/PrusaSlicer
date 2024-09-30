#include "Slic3r/App/Platform/PlatformInfo.hpp"

namespace Slic3r::App::Platform {



PlatformInfo& PlatformInfo::instance()
{
    static PlatformInfo inst;
    return inst;
}


}
