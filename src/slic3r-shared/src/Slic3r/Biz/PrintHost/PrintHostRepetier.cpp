#include "Slic3r/Biz/PrintHost/PrintHostRepetier.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

namespace {
bool validate_repetier(const boost::optional<std::string>& name, const boost::optional<std::string>& soft)
{
    if (soft) {
        // See https://github.com/prusa3d/PrusaSlicer/issues/7807:
        // Repetier allows "rebranding", so the "name" value is not reliable when detecting
        // server type. Newer Repetier versions send "software", which should be invariant.
        return ((*soft) == "Repetier-Server");
    } else {
        // If there is no "software" value, validate as we did before:
        return name ? boost::starts_with(*name, "Repetier") : true;
    }
}
}
bool PrintHostRepetier::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
     const char *name = get_name();

    const auto upload_filename = upload_data.dest_path.filename();
    const auto upload_parent_path = upload_data.dest_path.parent_path();

    std::string test_msg;
    if (! test(test_msg, retry_fn)) {
        error_fn(std::move(test_msg));
        return false;
    }

    bool res = true;

    auto url = upload_data.post_action == PrintHostAfterUploadAction::StartPrint
        ? make_url(format("printer/job/%1%", m_print_host_config.port))
        : make_url(format("printer/model/%1%", m_print_host_config.port));

    SPDLOG_INFO(format("%1%: Uploading file at %2%, filename: %3%, path: %4%, print: %5%, group: %6%"
        , name
        , url
        , upload_filename.string()
        , upload_parent_path.string()
        , (upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")
        , upload_data.group));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn);
    set_auth(http.get());

    if (!upload_data.group.empty() && upload_data.group != _u8L("Default")) {
        http->form_add("group", upload_data.group);
    }

    if(upload_data.post_action == PrintHostAfterUploadAction::StartPrint) {
        http->form_add("name", upload_filename.string());
        http->form_add("autostart", "true"); // See https://github.com/prusa3d/PrusaSlicer/issues/7807#issuecomment-1235519371
    }

    http->form_add("a", "upload")
        .form_add_data("filename", std::move(upload_data.raw_data), upload_filename)
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File uploaded: HTTP %2%: %3%", name , status , body));
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
            error_fn(format_error(body, error, status));
            res = false;
        })
        .on_progress([&](Network::IHttp::Progress progress, bool &cancel) {
            progress_fn(std::move(progress), cancel);
            if (cancel) {
                // Upload was canceled
                SPDLOG_INFO("Repetier: Upload canceled");
                res = false;
            }
        })
        .perform_sync();

    return res;
}

bool PrintHostRepetier::test(std::string& msg, RetryFn retry_fn) const
{
    // Since the request is performed synchronously here,
    // it is ok to refer to `msg` from within the closure

    const char *name = get_name();

    bool res = true;
    auto url = make_url("printer/info");

    SPDLOG_INFO(format("%1%: List version at: %2%", name , url));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    set_auth(http.get());
    
    http->on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error getting version: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
            res = false;
            msg = format_error(body, error, status);
        })
        .on_complete([&](std::string body, unsigned) {
            SPDLOG_INFO(format("%1%: Got version: %2%", name , body));

            try {
                nlohmann::json json = nlohmann::json::parse(body);
                boost::optional<std::string> text;
                if (json.contains("name") && json["name"].is_string()) {
                   text = json["name"];
                }
                boost::optional<std::string> soft;
                if (json.contains("software") && json["software"].is_string()) {
                   soft = json["software"];
                }
                res = validate_repetier(text, soft);
                if (! res) {
                    msg = format(_u8L("Mismatched type of print host: %s"), (soft ? *soft : (text ? *text : "Repetier")));
                }
            }
            catch (const std::exception &) {
                res = false;
                msg = "Could not parse server response";
            }
        })
        .perform_sync();

    return res;
}

std::string PrintHostRepetier::make_url(const std::string &path) const
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

void PrintHostRepetier::set_auth(Network::IHttp* http) const
{
    http->header("X-Api-Key", m_print_host_config.api_key);

    if (! m_print_host_config.ca_file.empty()) {
        http->ca_file(m_print_host_config.ca_file);
    }
}


} // namespace Slic3r::Biz::PrintHost