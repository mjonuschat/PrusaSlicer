#include "Slic3r/Biz/PrintHost/PrintHostPrusaConnect.hpp"

#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/nowide/convert.hpp>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

namespace
{

boost::optional<std::string> get_error_message_from_response_body(const std::string& body)
{
    boost::optional<std::string> message;
    try
    {
        nlohmann::json json = nlohmann::json::parse(body);
        if (json.contains("message") && json["message"].is_string()) {
            message = json["message"];
        }
    }
    // ignore possible errors if body is not valid JSON
    catch (std::exception&)
    {
    }

    return message;
}
// This is not needed anymore, but might come handy when implementing User Account later. If 3.0 is released, this can be removed.
#if 0
std::string parse_json_for_param(const nlohmann::json& json_obj, const std::string& param) {
    if (json_obj.contains(param) && json_obj[param].is_string())
    {
        return json_obj[param];
    }
    for (auto it = json_obj.begin(); it != json_obj.end(); ++it) {
        if (it->is_structured()) {
            if (std::string res = parse_json_for_param(*it, param); !res.empty()) {
                return res;
            }
        }
    }
    return {};
}

std::string get_keyword_from_json(const std::string& body, const std::string& keyword ) 
{
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(body);
    } catch (const std::exception &e) {
        SPDLOG_ERROR(format("Failed to parse json: %1%", e.what()));
        SPDLOG_ERROR(json);
        return {};
    }
    return parse_json_for_param(json, keyword);
}
#endif
}

bool PrintHostPrusaConnect::test(std::string& curl_msg, RetryFn retry_fn) const
{
    // Test is not used by upload and gets list of files on a device.   
    const std::string name = get_name();
    std::string url = format("%1%/%2%/files?printer_uuid=%3%", Network::ServiceConfig::instance().connect_teams_url(), m_print_host_config.team_id, m_print_host_config.printer_uuid);
    SPDLOG_INFO(format("%1%: Get files/raw at: %2%", name, url));
    bool res = true;
  
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    http->header("Authorization", "Bearer " + m_print_host_config.access_token);
    http->on_error([&](std::string body, std::string error, unsigned status) {
        SPDLOG_ERROR(format("%1%: Error getting version: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
        res = false;
        curl_msg = format_error(body, error, status);
    })
    .on_complete([&](std::string body, unsigned) {
         SPDLOG_INFO(format("%1%: Got files/raw: %2%", name , body));
    })
    .perform_sync();

    return res;
    
}

bool PrintHostPrusaConnect::init_upload(const PrintHostJobData& upload_data, std::string& out, RetryFn retry_fn) const
{
    // Register upload. Then upload must be performed immediately with returned "id" 
    bool res = true;
    boost::system::error_code ec;
    boost::uintmax_t size = upload_data.raw_data.size();
    const std::string name = get_name();
    const std::string upload_filename = upload_data.dest_path.filename().string();
    std::string url = format("%1%/app/users/teams/%2%/uploads", m_print_host_config.host, m_print_host_config.team_id);
    std::string request_body_json = upload_data.request_body_json;
    
    // replace placeholder filename 
    ASSERT(request_body_json.find("%1%") != std::string::npos);
    ASSERT(request_body_json.find("%2%") != std::string::npos);
    request_body_json = format(request_body_json, upload_filename, size);
    
    SPDLOG_INFO("Register upload to " + name + ". Url: " + url + "\nBody: " + request_body_json);
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn);
    http->header("Authorization", "Bearer " + m_print_host_config.access_token)
        .header("Content-Type", "application/json")
        .set_post_body(std::move(request_body_json))
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File upload registered: HTTP %2%: %3%", name , status , body));
            out = body;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error registering file: %2%, HTTP %3%, body: `%4%`", name , error , status ,body));
            res = false;
            out = get_error_message_from_response_body(body).value_or_eval([&](){
                return format_error(body, error, status);
            });
        })
        .perform_sync();
    return res;
}

bool PrintHostPrusaConnect::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    std::string json = format(upload_data.request_body_json, "", "1");
    std::string printer_page_url = format("%1%/printer/%2%/dashboard", Network::ServiceConfig::instance().connect_url(), m_print_host_config.printer_uuid);
    info_fn("prusaconnect_printer_address", printer_page_url);

    std::string init_out;
    if (!init_upload(upload_data, init_out, retry_fn))
    {
        error_fn(init_out);
        return false;
    }
 
    // init reply format: {"id": 1234, "team_id": 12345, "name": "filename.gcode", "size": 123, "hash": "QhE0LD76vihC-F11Jfx9rEqGsk4.", "state": "INITIATED", "source": "CONNECT_USER", "path": "/usb/filename.bgcode"}
    std::string upload_id;
    try
    {
        nlohmann::json json = nlohmann::json::parse(init_out);
        if (!json.contains("id") || !json["id"].is_string()) {
            error_fn(_u8L("Failed to extract upload id from server reply."));
            return false;
        }
        upload_id = json["id"];
    }
    catch (const std::exception&)
    {
        error_fn(_u8L("Failed to extract upload id from server reply."));
        return false;
    }
    const std::string name = get_name();
    const std::string url = format(
        "%1%/app/teams/%2%/files/raw"
        "?upload_id=%3%"
        , Network::ServiceConfig::instance().connect_url(), m_print_host_config.team_id, upload_id);
    bool res = true;


    SPDLOG_INFO(format("%1%: Uploading file at %2%, filename: %3%, path: %4%, print: %5%"
        , name
        , url
        , upload_data.dest_path.filename().string()
        , upload_data.dest_path.parent_path().string()
        , (upload_data.post_action == PrintHostAfterUploadAction::StartPrint ? "true" : "false")));
     
    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Put, std::move(url), retry_fn);
    http->set_put_data(std::move(upload_data.raw_data), upload_data.dest_path)
        .header("Content-Type", "text/x.gcode")
        .header("Authorization", "Bearer " + m_print_host_config.access_token)
        .on_complete([&](std::string body, unsigned status) {
            SPDLOG_INFO(format("%1%: File uploaded: HTTP %2%: %3%", name , status , body));
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            SPDLOG_ERROR(format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`", name , error , status , body));
            error_fn(format_error(body, error, status));
            res = false;
        })
        .on_progress([&](Network::IHttp::Progress progress, bool& cancel) {
            progress_fn(std::move(progress), cancel);
            if (cancel) {
                // Upload was canceled
                SPDLOG_INFO("PrusaConnect: Upload canceled");
                res = false;
            }
        })
        .perform_sync();

    return res;
}

} // namespace Slic3r::Biz::PrintHost