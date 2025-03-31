#include "Slic3r/Biz/PrintHost/PrintHostOctoPrint.hpp"

#include "Slic3r/Biz/Network/Bonjour.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/Log.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>

#include <utility>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

PrintHostOctoPrint::PrintHostOctoPrint(PrintHostConfig config) 
    : IPrintHost(std::move(config))
{
}

bool PrintHostOctoPrint::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
#ifndef WIN32
    return upload_inner_with_host(std::move(upload_data), progress_fn, retry_fn, error_fn, info_fn);
#else
    std::string host = Network::IHttp::extract_host_from_url(m_print_host_config.host);

    // decide what to do based on host - resolve hostname or upload to ip
    std::vector<boost::asio::ip::address> resolved_addr;
    boost::system::error_code ec;
    boost::asio::ip::address host_ip = boost::asio::ip::make_address(host, ec);
    if (!ec) {
        resolved_addr.push_back(host_ip);
    } else if (boost::algorithm::ends_with(host, ".local")) {
        Bonjour("octoprint")
            .set_hostname(host)
            .set_retries(5) // number of rounds of queries send
            .set_timeout(1) // after each timeout, if there is any answer, the resolving will stop
            .on_resolve([&ra = resolved_addr](const std::vector<BonjourReply>& replies) {
                for (const auto & rpl : replies) {
                    boost::asio::ip::address ip(rpl.ip);
                    ra.emplace_back(ip);
                    SPDLOG_INFO(format("Resolved IP address: %1%", rpl.ip));
                }
            })
            .resolve_sync();
    }

    if (resolved_addr.empty()) {
        // no resolved addresses - try system resolving
        SPDLOG_ERROR(format("Failed to resolve hostname %1% into the IP address. Starting upload with system resolving.", m_print_host_config.host));
        return upload_inner_with_host(std::move(upload_data), progress_fn, retry_fn, error_fn, info_fn);
    } else if (resolved_addr.size() == 1) {
        // one address resolved - upload there
        return upload_inner_with_resolved_ip(std::move(upload_data), progress_fn, retry_fn, error_fn, info_fn, resolved_addr.front());
    }  else if (resolved_addr.size() == 2 && resolved_addr[0].is_v4() != resolved_addr[1].is_v4()) {
        // there are just 2 addresses and 1 is ip_v4 and other is ip_v6
        // try sending to both. (Then if both fail, show both error msg after second try)
        std::string error_message;
        if (!upload_inner_with_resolved_ip(std::move(upload_data), progress_fn, retry_fn
            , [&msg = error_message, resolved_addr](std::string error) { msg = format("%1%: %2%", resolved_addr.front(), error); }
            , info_fn, resolved_addr.front())
            &&
            !upload_inner_with_resolved_ip(std::move(upload_data), progress_fn, retry_fn
            , [&msg = error_message, resolved_addr](std::string error) { msg += format("\n%1%: %2%", resolved_addr.back(), error); }
            , info_fn, resolved_addr.back())
            ) {

            error_fn(error_message);
            return false;
        }
        return true;
    } else {
        // There are multiple addresses - user needs to choose which to use. (Here used to be dialog (We are in worker thread!!))
        // Lets try all now until some works?
        for (size_t i = 0; i < resolved_addr.size(); i++) {
            if (upload_inner_with_resolved_ip(std::move(upload_data), progress_fn, retry_fn, error_fn, info_fn, resolved_addr[i])) {
                return true;
            }
        }
    }
    return false;
#endif // WIN32
}


bool PrintHostOctoPrint::test(std::string& msg, RetryFn retry_fn) const
{
    const char* name = get_name();
    bool res = true;
    auto url = make_url("api/version");

    SPDLOG_INFO(format("%1%: Get version at: %2%", name , url));
    // Here we do not have to add custom "Host" header - the url contains host filled by user and IHttp will set the header by itself.
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    set_auth(http.get());
    http->on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error getting version: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
            res = false;
            msg = format_error(body, error, status);
        })
        .on_complete([&, this](std::string body, unsigned) {
            SPDLOG_INFO(format("%1%: Got version: %2%", name , body));

            try {
                nlohmann::json json = nlohmann::json::parse(body);
                if (!json.contains("api") || !json["api"].is_string()) {
                    res = false;
                    return;
                }
                boost::optional<std::string> text;
                if (json.contains("text") && json["text"].is_string()) {
                   text = json["text"];
                }
                res = validate_version_text(text);
                if (! res) {
                    msg = format(_u8L("Mismatched type of print host: %s"), (text ? *text : name));
                }
            }
            catch (const std::exception &) {
                res = false;
                msg = "Could not parse server response";
            }
        })
#ifdef WIN32
        .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
        .on_ip_resolve([&](std::string address) {
            // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
            // Remember resolved address to be reused at successive REST API call.
            msg = address;
        })
#endif // WIN32
        .perform_sync();

    return res;
}

#ifdef WIN32
bool PrintHostOctoPrint::test_with_resolved_ip(std::string& msg, RetryFn retry_fn) const
{
    // Since the request is performed synchronously here,
    // it is ok to refer to `msg` from within the closure
    const char* name = get_name();
    bool res = true;
    // Msg contains ip string.
    std::string url = Network::IHttp::substitute_host(make_url("api/version"), msg);
    msg.clear();

    SPDLOG_INFO(format("%1%: Get version at: %2%", name , url));

    std::string host = Network::IHttp::extract_host_from_url(m_print_host_config.host);
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, url, retry_fn);
    // "Host" header is necessary here. We have resolved IP address and substituted it into "url" variable.
    // And when creating Http object above, libcurl automatically includes "Host" header from address it got.
    // Thus "Host" is set to the resolved IP instead of host filled by user. We need to change it back.
    // Not changing the host would work on the most cases (where there is 1 service on 1 hostname) but would break when f.e. reverse proxy is used (issue #9734).
    // Also when allow_ip_resolve = 0, this is not needed, but it should not break anything if it stays.
    // https://www.rfc-editor.org/rfc/rfc7230#section-5.4
    http->header("Host", host);
    set_auth(http.get());
    http->on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error getting version at %2% : %3%, HTTP %4%, body: `%5%`", name , url , error , status , body));
            res = false;
            msg = format_error(body, error, status);
        })
        .on_complete([&](std::string body, unsigned) {
            SPDLOG_INFO(format("%1%: Got version: %2%", name , body));

            try {
                nlohmann::json json = nlohmann::json::parse(body);
                if (!json.contains("api") || !json["api"].is_string()) {
                    res = false;
                    return;
                }
                boost::optional<std::string> text;
                if (json.contains("text") && json["text"].is_string()) {
                   text = json["text"];
                }
                res = validate_version_text(text);
                if (! res) {
                    msg = format(_u8L("Mismatched type of print host: %s"), (text ? *text : name));
                }
            }
            catch (const std::exception&) {
                res = false;
                msg = "Could not parse server response.";
            }
        })
        .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
        .perform_sync();

    return res;
}
#endif //WIN32


bool PrintHostOctoPrint::upload_inner_with_host(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    const char* name = get_name();

    const auto upload_filename = upload_data.dest_path.filename();
    const auto upload_parent_path = upload_data.dest_path.parent_path();

    // If test fails, test_msg contains the error message.
    // Otherwise on Windows it contains the resolved IP address of the host.
    std::string test_msg;
    if (!test(test_msg, retry_fn)) {
        error_fn(std::move(test_msg));
        return false;
    }

    std::string url;
    bool res = true;

#ifdef WIN32
    // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
    if (m_print_host_config.host.find("https://") == 0 || test_msg.empty())
#endif // _WIN32
    {
        // If https is entered we assume signed certificate is being used
        // IP resolving will not happen - it could resolve into address not being specified in cert
        url = make_url("api/files/local");
    }
#ifdef WIN32
    else {
        // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
        // Curl uses easy_getinfo to get ip address of last successful transaction.
        // If it got the address use it instead of the stored in "host" variable.
        // This new address returns in "test_msg" variable.
        // Solves troubles of uploades failing with name address.
        // in original address (m_host) replace host for resolved ip 
        info_fn("resolve", test_msg);
        url = Network::IHttp::substitute_host(make_url("api/files/local"), test_msg);
        SPDLOG_INFO("Upload address after ip resolve: ", url);
    }
#endif // _WIN32

    SPDLOG_INFO(format("%1%: Uploading file to %2%, filename: %3%, path: %4%, print: %5%"
        , name
        , url
        , upload_filename.string()
        , upload_parent_path.string()
        , (upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn);
#ifdef WIN32
    // "Host" header is necessary here. In the workaround above (two mDNS..) we have got IP address from test connection and subsituted it into "url" variable.
    // And when creating Http object above, libcurl automatically includes "Host" header from address it got.
    // Thus "Host" is set to the resolved IP instead of host filled by user. We need to change it back.
    // Not changing the host would work on the most cases (where there is 1 service on 1 hostname) but would break when f.e. reverse proxy is used (issue #9734).
    // Also when allow_ip_resolve = 0, this is not needed, but it should not break anything if it stays.
    // https://www.rfc-editor.org/rfc/rfc7230#section-5.4
    std::string host = Network::IHttp::extract_host_from_url(m_print_host_config.host);
    http->header("Host", host);
#endif // _WIN32
    set_auth(http.get());
    http->form_add("print", upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")
        .form_add("path", upload_parent_path.string()) 
        .form_add_data("file", std::move(upload_data.raw_data), upload_filename)
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File uploaded: HTTP %2%: %3%", name , status , body));
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_INFO(format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
            error_fn(format_error(body, error, status));
            res = false;
        })
        .on_progress([&](Network::IHttp::Progress progress, bool& cancel) {
            progress_fn(std::move(progress), cancel);
            if (cancel) {
                // Upload was canceled
                SPDLOG_INFO(format("%1%: Upload canceled", name));
                res = false;
            }
        })
#ifdef WIN32
        .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
#endif
        .perform_sync();

    return res;
}

#ifdef _WIN32
bool PrintHostOctoPrint::upload_inner_with_resolved_ip(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn, const boost::asio::ip::address& resolved_addr) const
{
    info_fn("resolve", resolved_addr.to_string());

    // If test fails, test_msg contains the error message.
    // Otherwise on Windows it contains the resolved IP address of the host.
    // Test_msg already contains resolved ip and will be cleared on start of test().
    std::string test_msg = resolved_addr.to_string();
    if (!test_with_resolved_ip(test_msg, retry_fn)) {
        error_fn(std::move(test_msg));
        return false;
    }

    const char* name = get_name();
    const auto upload_filename = upload_data.dest_path.filename();
    const auto upload_parent_path = upload_data.dest_path.parent_path();
    std::string url = Network::IHttp::substitute_host(make_url("api/files/local"), resolved_addr.to_string());
    bool result = true;

    info_fn("resolve", url);

    SPDLOG_INFO(format("%1%: Uploading file at %2%, filename: %3%, path: %4%, print: %5%"
        , name
        , url
        , upload_filename.string()
        , upload_parent_path.string()
        , (upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, url, retry_fn);
    // "Host" header is necessary here. We have resolved IP address and subsituted it into "url" variable.
    // And when creating Http object above, libcurl automatically includes "Host" header from address it got.
    // Thus "Host" is set to the resolved IP instead of host filled by user. We need to change it back.
    // Not changing the host would work on the most cases (where there is 1 service on 1 hostname) but would break when f.e. reverse proxy is used (issue #9734).
    // https://www.rfc-editor.org/rfc/rfc7230#section-5.4
    std::string host = Network::IHttp::extract_host_from_url(m_print_host_config.host);
    http->header("Host", host);
    set_auth(http.get());
    http->form_add("print", upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")
        .form_add("path", upload_parent_path.string())      // XXX: slashes on windows ???
        .form_add_data("file", std::move(upload_data.raw_data), upload_data.dest_path)
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File uploaded: HTTP %2%: %3%", name , status , body));
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error uploading file to %2%: %3%, HTTP %4%, body: `%5%`", name , url , error , status , body));
            error_fn(format_error(body, error, status));
            result = false;
        })
        .on_progress([&](Network::IHttp::Progress progress, bool& cancel) {
            progress_fn(std::move(progress), cancel);
            if (cancel) {
                // Upload was canceled
                SPDLOG_INFO(format("%1%: Upload canceled", name));
                result = false;
            }
        })
        .ssl_revoke_best_effort(m_print_host_config.ssl_revoke_best_effort)
        .perform_sync();

    return result;

}
#endif // _WIN32

std::string PrintHostOctoPrint::make_url(const std::string& path) const
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

void PrintHostOctoPrint::set_auth(Network::IHttp* http) const
{
    http->header("X-Api-Key", m_print_host_config.api_key);

    if (!m_print_host_config.ca_file.empty()) {
        http->ca_file(m_print_host_config.ca_file);
    }
}

bool PrintHostOctoPrint::validate_version_text(const boost::optional<std::string>& version_text) const
{
    return version_text ? boost::starts_with(*version_text, "OctoPrint") : true;
}


} // Slic3r::Biz::PrintHost 