#pragma once

#include <string>

namespace Slic3r::Biz::PrintHost {  
class IPrintHostJobCallbacks {
public:
    virtual ~IPrintHostJobCallbacks() = default;
    virtual void on_job_progress(size_t id, int progress) = 0;
    virtual void on_job_error(size_t id, const std::string& msg) = 0;
    virtual void on_job_cancel(size_t id) = 0;
    virtual void on_job_done(size_t id) = 0;
    virtual void on_job_info(size_t id, const std::string& tag, const std::string& msg) = 0;
};
} // namespace Slic3r::Biz::PrintHost