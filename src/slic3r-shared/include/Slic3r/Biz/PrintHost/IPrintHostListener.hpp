#pragma once

#include <boost/log/trivial.hpp>
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"

namespace Slic3r::Biz::PrintHost {


class IPrintHostListener {
public:
    virtual ~IPrintHostListener() = default;
    virtual void on_print_host_progress(size_t id, int progress) = 0;
    virtual void on_print_host_error(size_t id, const std::string& msg) = 0;
    virtual void on_print_host_cancel(size_t id) = 0;
    virtual void on_print_host_done(size_t id) = 0;
    virtual void on_print_host_info(size_t id, const std::string& tag, const std::string& msg) = 0;
};

class IPrintHostBinarizeListener {
public:
    virtual ~IPrintHostBinarizeListener() = default;
    virtual void on_print_host_binarize_success(PrintHostConfig config, PrintHostJobData data) = 0;
    virtual void on_print_host_binarize_fail(const std::string& msg) = 0;
};

} // namespace Slic3r::Biz::PrintHost