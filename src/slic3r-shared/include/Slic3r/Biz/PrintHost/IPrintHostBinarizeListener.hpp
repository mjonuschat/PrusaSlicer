#pragma once

#include <boost/log/trivial.hpp>
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"

namespace Slic3r::Biz::PrintHost {

class IPrintHostBinarizeListener {
public:
    virtual ~IPrintHostBinarizeListener() = default;
    virtual void on_print_host_binarize_success(PrintHostConfig config, PrintHostJobData data) = 0;
    virtual void on_print_host_binarize_fail(const std::string& msg) = 0;
};

} // namespace Slic3r::Biz::PrintHost