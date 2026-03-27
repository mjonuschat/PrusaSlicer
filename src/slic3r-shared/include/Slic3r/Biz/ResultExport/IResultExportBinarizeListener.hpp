#pragma once

#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"

namespace Slic3r::Biz::ResultExport {

class IResultExportBinarizeListener {
public:
    virtual ~IResultExportBinarizeListener() = default;
    virtual void on_result_export_binarize_success(PrintHost::PrintHostConfig config, PrintHost::PrintHostJobData data) = 0;
    virtual void on_result_export_binarize_fail(const std::string& msg) = 0;
};

} // namespace Slic3r::Biz::PrintHost