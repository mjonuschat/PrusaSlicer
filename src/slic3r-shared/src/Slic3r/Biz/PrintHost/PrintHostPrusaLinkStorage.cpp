#include "Slic3r/Biz/PrintHost/PrintHostPrusaLinkStorage.hpp"

#include "Slic3r/Log.hpp"

#include "libslic3r/format.hpp"

#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

PrintHostPrusaLinkStorage::PrintHostPrusaLinkStorage(PrintHostConfig config)
    : IPrintHost(std::move(config))
{}

bool PrintHostPrusaLinkStorage::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    
    const char* name = get_name();

    bool res = true;
    auto url = make_url("api/v1/storage");
    std::string error_msg;
    std::string storage_result;

    SPDLOG_INFO(format("%1%: Get storage at: %2%", name, url));

    //TODO: get language from somewhere
    std::string lang = "en";
    
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    set_auth(http.get());
    http->header("Accept-Language", lang);
    http->on_error([&](std::string body, std::string error, unsigned status) {
        SPDLOG_ERROR(format("%1%: Error getting storage: %2%, HTTP %3%, body: `%4%`", name, error, status, body));
        error_msg = "\n\n" + error;
        res = false;
        // If status is 0, the communication with the printer has failed completely (most likely a timeout), if the status is <= 400, it is an error returned by the printer.
        // If 0, we can show error to the user now, as we know the communication has failed.
        // if not 0, we must not show error, as not all printers support api/v1/storage endpoint.
        // So we must be extra careful here, or we might be showing errors on perfectly fine communication.
        if (status != 0)
            res = true;
    })
    .on_complete([&](std::string body, unsigned) {
        SPDLOG_INFO(format("%1%: Got storage: %2%", name, body));
        try {
            nlohmann::json json = nlohmann::json::parse(body);

            if (!json.contains("storage_list")) {
                res = false;
                return;
            }
            storage_result = json["storage_list"].dump();
        } catch (const std::exception&) {
            res = false;
        }
        
    })
#ifdef WIN32
    .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)     
#endif // WIN32
    .perform_sync();
    
    if (res)
    {
        info_fn("storage", storage_result);
    }

    return res;
}

std::string PrintHostPrusaLinkStorage::make_url(const std::string& path) const
{
    if (m_print_host_config.host.find("http://") == 0 || m_print_host_config.host.find("https://") == 0) {
        if (m_print_host_config.host.back() == '/') {
            return format("%1%%2%", m_print_host_config.host, path);
        } else {
            return format("%1%/%2%", m_print_host_config.host, path);
        }
    } else {
        return format("http://%1%/%2%", m_print_host_config.host, path);
    }
}

void PrintHostPrusaLinkStorage::set_auth(Network::IHttp* http) const
{
    switch (m_print_host_config.auth_type) {
    case PrintHostAuthType::ApiKey:
        http->header("X-Api-Key", m_print_host_config.api_key);
        break;
    case PrintHostAuthType::Digest:
        http->auth_digest(m_print_host_config.username, m_print_host_config.password);
        break;
    default:
        ASSERT(false, "PrusaLink does not support other auth method than api key or http digest.");
        break;
    }

    if (!m_print_host_config.ca_file.empty()) {
        http->ca_file(m_print_host_config.ca_file);
    }
}
}