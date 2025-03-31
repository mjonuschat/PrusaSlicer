#pragma once

#include <Slic3r/Biz/PrintHost/IPrintHost.hpp>
#include <Slic3r/Biz/PrintHost/PrintHostConfig.hpp>

namespace Slic3r::Biz::PrintHost {  

std::unique_ptr<IPrintHost> create_print_host(PrintHostConfig config);  

} // namespace Slic3r::Biz::PrintHost