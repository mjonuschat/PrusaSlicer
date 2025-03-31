#pragma once 

#include <Slic3r/Biz/PrintHost/IPrintHost.hpp>

namespace boost::filesystem { class path; }

namespace Slic3r::Biz::PrintHost {

class PrintHostLocal : public IPrintHost {

public:
    PrintHostLocal(PrintHostConfig config) : IPrintHost(std::move(config)) {}
    
    PrintHostLocal(const PrintHostLocal&) = delete;
    PrintHostLocal& operator=(const PrintHostLocal&) = delete;
    PrintHostLocal(PrintHostLocal&& other) noexcept = default;
    PrintHostLocal& operator=(PrintHostLocal&& other) noexcept = default;
   
    ~PrintHostLocal() override {}

    
    bool perform(PrintHostJobData upload_data,  ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const override;

    const char* get_name() const override { return "Local Export"; }
    bool test(std::string& msg, RetryFn retry_fn) const override { return true; }
protected:

    bool write_file(const std::string& data, const boost::filesystem::path& path, std::string& msg) const;
};
} // namespace Slic3r::Biz::PrintHost