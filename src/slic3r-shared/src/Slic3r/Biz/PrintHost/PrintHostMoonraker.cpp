#include "Slic3r/Biz/PrintHost/PrintHostMoonraker.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

bool PrintHostMoonraker::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    // POST /server/files/upload

    const char* name = get_name();
    const auto upload_filename = upload_data.dest_path.filename();
    const auto upload_parent_path = upload_data.dest_path.parent_path();

    // If test fails, test_msg_or_host_ip contains the error message.
    std::string test_msg_or_host_ip;
    if (!test(test_msg_or_host_ip, retry_fn)) {
        error_fn(std::move(test_msg_or_host_ip));
        return false;
    }

    std::string url;
    bool res = true;

#ifdef WIN32
    // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
    if (m_print_host_config.host.find("https://") == 0 || test_msg_or_host_ip.empty())
#endif // _WIN32
    {
        // If https is entered we assume signed certificate is being used
        // IP resolving will not happen - it could resolve into address not being specified in cert
        url = make_url("server/files/upload");
    }
#ifdef WIN32
    else {
        // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
        // Curl uses easy_getinfo to get ip address of last successful transaction.
        // If it got the address use it instead of the stored in "host" variable.
        // This new address returns in "test_msg_or_host_ip" variable.
        // Solves troubles of uploads failing with name address.
        // in original address (m_host) replace host for resolved ip 
        info_fn("resolve", test_msg_or_host_ip);
        url = Network::IHttp::substitute_host(make_url("server/files/upload"), test_msg_or_host_ip);
        SPDLOG_INFO("Upload address after ip resolve: " + url);
    }
#endif // _WIN32

    SPDLOG_INFO(format("%1%: Uploading file at %2%, filename: %3%, path: %4%, print: %5%"
        , name
        , url
        , upload_filename.string()
        , upload_parent_path.string()
        , (upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")));
    /*
    The file must be uploaded in the request's body multipart/form-data (ie: <input type="file">). The following arguments may also be added to the form-data:
    root: The root location in which to upload the file.Currently this may be gcodes or config.If not specified the default is gcodes.
    path : This argument may contain a path(relative to the root) indicating a subdirectory to which the file is written.If a path is present the server will attempt to create any subdirectories that do not exist.
    checksum : A SHA256 hex digest calculated by the client for the uploaded file.If this argument is supplied the server will compare it to its own checksum calculation after the upload has completed.A checksum mismatch will result in a 422 error.
    Arguments available only for the gcodes root :
    print: If set to "true", Klippy will attempt to start the print after uploading.Note that this value should be a string type, not boolean.This provides compatibility with OctoPrint's upload API.
    */
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn);
    set_auth(http.get());
    
    http->form_add("root", "gcodes");
    if (!upload_parent_path.empty())
        http->form_add("path", upload_parent_path.string());
    if (upload_data.post_action == PrintHostAfterUploadAction::StartPrint)
        http->form_add("print", "true");
   
    http->form_add_data("file", std::move(upload_data.raw_data), upload_filename)
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File uploaded: HTTP %2%: %3%", name , status , body));
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`", name , error , status ,body));
            error_fn(format_error(body, error, status));
            res = false;
        })
        .on_progress([&](Network::IHttp::Progress progress, bool& cancel) {
            progress_fn(std::move(progress), cancel);
            if (cancel) {
                // Upload was canceled
                SPDLOG_INFO("Moonraker: Upload canceled");
                res = false;
            }
        })
#ifdef WIN32
        .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
#endif
        .perform_sync();

    return res;
}

bool PrintHostMoonraker::test(std::string& msg, RetryFn retry_fn) const
{
    // GET /server/info

    // Since the request is performed synchronously here,
    // it is ok to refer to `msg` from within the closure
    const char* name = get_name();

    bool res = true;
    auto url = make_url("server/info");

    SPDLOG_INFO(format("%1%: Get version at: %2%", name , url));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    set_auth(http.get());
    http->on_error([&](std::string body, std::string error, unsigned status) {
        SPDLOG_ERROR(format("%1%: Error getting version: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
        res = false;
        msg = format_error(body, error, status);
    })
    .on_complete([&](std::string body, unsigned) {
        SPDLOG_INFO(format("%1%: Got server/info: %2%", name , body));
            
        try {
            // All successful HTTP requests will return a json encoded object in the form of :
            // {result: <response data>}
            nlohmann::json json = nlohmann::json::parse(body);
            if (!json.contains("result") || !json["result"].is_structured()) {
                msg = "Could not parse server response";
                res = false;
                return;
            }
            if (!json["result"].contains("moonraker_version") || !json["result"]["moonraker_version"].is_string()) {
                msg = "Could not parse server response";
                res = false;
                return;
            }
            SPDLOG_INFO(format("%1%: Got version: %2%", name , json["result"]["moonraker_version"]));
        } catch (const std::exception&) {
            res = false;
            msg = "Could not parse server response";
        }
    })
#ifdef _WIN32
    .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
    .on_ip_resolve([&](std::string address) {
        // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
        // Remember resolved address to be reused at successive REST API call.
        msg =address;
    })
#endif // _WIN32
    .perform_sync();

    return res;
}

std::string PrintHostMoonraker::make_url(const std::string& path) const
{
    if (m_print_host_config.host.find("http://") == 0 || m_print_host_config.host.find("https://") == 0) {
        if (m_print_host_config.host.back() == '/') {
            return format("%1%%2%", m_print_host_config.host , path);
        } else {
            return format("%1%/%2%", m_print_host_config.host , path);
        }
    } else {
        return format("http://%1%/%2%", m_print_host_config.host , path);
    }
}

void PrintHostMoonraker::set_auth(Network::IHttp* http) const
{
    switch (m_print_host_config.auth_type) {
    case PrintHostAuthType::ApiKey:
        http->header("X-Api-Key", m_print_host_config.api_key);
        break;
    case PrintHostAuthType::Digest:
        http->auth_digest(m_print_host_config.username, m_print_host_config.password);
        break;
    default:
        break;
    }

    if (!m_print_host_config.ca_file.empty()) {
        http->ca_file(m_print_host_config.ca_file);
    }
}

} // namespace Slic3r::Biz::PrintHost