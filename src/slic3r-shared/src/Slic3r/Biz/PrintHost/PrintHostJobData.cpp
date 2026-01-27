#include "Slic3r/Biz/PrintHost/PrintHostJobData.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::Biz::PrintHost {

PrintHostExportFormat get_export_format_from_extension(const std::string& extension)
{
    SPDLOG_INFO("{} extension: {}", __FUNCTION__, extension);
    if (extension == ".gcode") 
        return PrintHostExportFormat::GCode;
    if (extension == ".bgcode") {
        return PrintHostExportFormat::BGCode;
    }
    if (extension == ".sl1") {
        return PrintHostExportFormat::Sl1;
    }
    if (extension == ".sl1s") {
        return PrintHostExportFormat::Sl1s;
    }
    ASSERT(false, "Unknown data format. Add it to PrintHostResultFormat");
    return PrintHostExportFormat::Undefined;
}

} // namespace Slic3r::Biz::PrintHost