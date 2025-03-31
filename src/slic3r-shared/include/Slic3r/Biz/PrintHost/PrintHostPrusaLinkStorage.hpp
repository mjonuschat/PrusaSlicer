#pragma once

#include "Slic3r/Biz/PrintHost/IPrintHost.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"

#include <boost/optional.hpp>


namespace Slic3r::Biz::PrintHost {

class PrintHostPrusaLinkStorage : public IPrintHost {
public:
    PrintHostPrusaLinkStorage(PrintHostConfig config);
    
    PrintHostPrusaLinkStorage(const PrintHostPrusaLinkStorage&) = delete;
    PrintHostPrusaLinkStorage& operator=(const PrintHostPrusaLinkStorage&) = delete;
    PrintHostPrusaLinkStorage(PrintHostPrusaLinkStorage&& other) noexcept = default;
    PrintHostPrusaLinkStorage& operator=(PrintHostPrusaLinkStorage&& other) noexcept = default;
   
    ~PrintHostPrusaLinkStorage() override = default;

    /**
     * Returns retrieved Storage via InfoFn. 
     */
    bool perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const override;

    const char* get_name() const override { return "PrusaLink"; }
    bool test(std::string& msg, RetryFn retry_fn) const override {return false; }
private:
    std::string make_url(const std::string& path) const;
    void set_auth(Network::IHttp* http) const;
};

}