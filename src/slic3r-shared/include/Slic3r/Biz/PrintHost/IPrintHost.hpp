#pragma once

#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"
#include "Slic3r/Assert.hpp"

#include <string>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

namespace Slic3r::Biz::PrintHost {

class IPrintHost
{
public:    
    explicit IPrintHost(PrintHostConfig config) : m_print_host_config(std::move(config)) {}
    
    IPrintHost() = delete;
    IPrintHost(const IPrintHost&) = delete;
    IPrintHost& operator=(const IPrintHost&) = delete;
    IPrintHost(IPrintHost&& other) noexcept = default;
    IPrintHost& operator=(IPrintHost&& other) noexcept = default;
    
    virtual ~IPrintHost() {}

    std::string get_host() const { return m_print_host_config.host; }

    typedef Network::IHttp::ProgressFn ProgressFn;
    typedef Network::IHttp::RetryFn RetryFn;
    typedef std::function<void(std::string /* error */)> ErrorFn;
    typedef std::function<void(std::string /* tag */, std::string /* status */)> InfoFn;

    /**
     * Delivers data to given path on host specified in config.
     */
    virtual bool perform(PrintHostJobData data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const = 0;

    virtual const char* get_name() const = 0;
    /**
     * Tests connection to remote host. Most PrintHosts uses test during perform as well.
     */
    virtual bool test(std::string& msg, RetryFn retry_fn) const = 0;
    
protected:
    PrintHostConfig m_print_host_config;

    virtual std::string format_error(const std::string &body, const std::string &error, unsigned status) const
    {
        if (status != 0) {
            return (boost::format("HTTP %1%: %2%") % status % body).str();
        } else {
            return error;
        }

    }
};

} // Slic3r::Biz::PrintHost