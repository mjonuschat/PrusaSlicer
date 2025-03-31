#pragma once

#include <Slic3r/Biz/PrintHost/IPrintHost.hpp>

namespace boost::filesystem { class path; }

namespace Slic3r::Biz::PrintHost {

class PrintHostMoonraker : public IPrintHost {

public:
    PrintHostMoonraker(PrintHostConfig config) : IPrintHost(std::move(config)) {}
    
    PrintHostMoonraker(const PrintHostMoonraker&) = delete;
    PrintHostMoonraker& operator=(const PrintHostMoonraker&) = delete;
    PrintHostMoonraker(PrintHostMoonraker&& other) noexcept = default;
    PrintHostMoonraker& operator=(PrintHostMoonraker&& other) noexcept = default;
   
    ~PrintHostMoonraker() override {}

    
    bool perform(PrintHostJobData upload_data,  ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const override;

    const char* get_name() const override { return "Moonraker"; }
    bool test(std::string& msg, RetryFn retry_fn) const override;
private:
    std::string make_url(const std::string& path) const;
    void set_auth(Network::IHttp* http) const;
};
} // namespace Slic3r::Biz::PrintHost