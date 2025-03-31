#include "Slic3r/Biz/PrintHost/PrintHostDuet.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

bool PrintHostDuet::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    std::string connect_msg;
	auto connectionType = connect(connect_msg, retry_fn);
	if (connectionType == ConnectionType::error) {
		error_fn(std::move(connect_msg));
		return false;
	}

	bool res = true;
	bool dsf = (connectionType == ConnectionType::dsf);

	auto upload_cmd = get_upload_url(upload_data.dest_path.string(), connectionType);
	SPDLOG_INFO(format("Duet: Uploading file. filepath: %1%, post_action: %2%, command: %3%"
		, upload_data.dest_path
		, int(upload_data.post_action)
		, upload_cmd));

     std::unique_ptr<Network::IHttp> http = (dsf 
        ? Network::IHttp::create(Network::IHttp::RequestMethod::Put, std::move(upload_cmd), retry_fn)
        : Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(upload_cmd), retry_fn));
	if (dsf) {
        http->set_put_data(std::move(upload_data.raw_data), upload_data.dest_path);
		if (connect_msg.empty())
            http->header("X-Session-Key", connect_msg);
	} else {
		http->set_post_body(std::move(upload_data.raw_data));
	}
	http->on_complete([&](std::string body, unsigned status) {
			SPDLOG_INFO(format("Duet: File uploaded: HTTP %1%: %2%", status , body));

			int err_code = dsf ? (status == 201 ? 0 : 1) : get_err_code_from_body(body);
			if (err_code != 0) {
				SPDLOG_INFO(format("Duet: Request completed but error code was received: %1%", err_code));
				error_fn(format_error(body, L("Unknown error occured"), 0));
				res = false;
			} else if (upload_data.post_action == PrintHostAfterUploadAction::StartPrint) {
				std::string errormsg;
				res = start_print(errormsg, upload_data.dest_path.string(), connectionType, false, retry_fn);
				if (! res) {
					error_fn(std::move(errormsg));
				}
			} else if (upload_data.post_action == PrintHostAfterUploadAction::StartSimulation) {
				std::string errormsg;
				res = start_print(errormsg, upload_data.dest_path.string(), connectionType, true, retry_fn);
				if (! res) {
					error_fn(std::move(errormsg));
				}
			}
		})
		.on_error([&](std::string body, std::string error, unsigned status) {
			SPDLOG_ERROR(format("Duet: Error uploading file: %1%, HTTP %2%, body: `%3%`", error , status , body));
			error_fn(format_error(body, error, status));
			res = false;
		})
		.on_progress([&](Network::IHttp::Progress progress, bool &cancel) {
			progress_fn(std::move(progress), cancel);
			if (cancel) {
				// Upload was canceled
				SPDLOG_INFO("Duet: Upload canceled");
				res = false;
			}
		})
		.perform_sync();

	disconnect(connectionType, retry_fn);
    return res;
}

bool PrintHostDuet::test(std::string& msg, RetryFn retry_fn) const
{
    auto connectionType = connect(msg, retry_fn);
	disconnect(connectionType, retry_fn);

	return connectionType != ConnectionType::error;
}

PrintHostDuet::ConnectionType PrintHostDuet::connect(std::string &msg, RetryFn retry_fn) const
{
	auto res = ConnectionType::error;
	auto url = get_connect_url(false);

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
	http->on_error([&](std::string body, std::string error, unsigned status) {
			auto dsfUrl = get_connect_url(true);
            std::unique_ptr<Network::IHttp> dsfHttp = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(dsfUrl), retry_fn);
			dsfHttp->on_error([&](std::string body, std::string error, unsigned status) {
					SPDLOG_ERROR(format("Duet: Error connecting: %1%, HTTP %2%, body: `%3%`", error , status , body));
					msg = format_error(body, error, status);
				})
				.on_complete([&](std::string body, unsigned) {
					try {		
                        nlohmann::json json = nlohmann::json::parse(body);
                        if (json.contains("sessionKey") && json["sessionKey"].is_string())
                        {
                            msg = json["sessionKey"];
                        }
                        res = ConnectionType::dsf;
					}
					catch (const std::exception&) {
						SPDLOG_ERROR(format("Failed to parse serverKey from Duet reply to Connect request: ", body));
						msg = format_error(body, L("Failed to parse a Connect reply"), 0);
						res = ConnectionType::error;
					}
				})
				.perform_sync();
		})
		.on_complete([&](std::string body, unsigned) {
			SPDLOG_INFO(format("Duet: Got: %1%", body));

			int err_code = get_err_code_from_body(body);
			switch (err_code) {
				case 0:
					res = ConnectionType::rrf;
					break;
				case 1:
					msg = format_error(body, L("Wrong password"), 0);
					break;
				case 2:
					msg = format_error(body, L("Could not get resources to create a new connection"), 0);
					break;
				default:
					msg = format_error(body, L("Unknown error occured"), 0);
					break;
			}

		})
		.perform_sync(); 

	return res;
}

void PrintHostDuet::disconnect(PrintHostDuet::ConnectionType connectionType, RetryFn retry_fn) const
{
	// we don't need to disconnect from DSF or if it failed anyway
	if (connectionType != ConnectionType::rrf) {
		return;
	}
	auto url =  (boost::format("%1%rr_disconnect")
			% get_base_url()).str();

	std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
	http->on_error([&](std::string body, std::string error, unsigned status) {
		// we don't care about it, if disconnect is not working Duet will disconnect automatically after some time
		SPDLOG_ERROR(format("Duet: Error disconnecting: %1%, HTTP %2%, body: `%3%`", error , status , body));
	})
	.perform_sync();
}

std::string PrintHostDuet::get_upload_url(const std::string &filename, PrintHostDuet::ConnectionType connectionType) const
{
    assert(connectionType != ConnectionType::error);

	if (connectionType == ConnectionType::dsf) {
		return format("%1%machine/file/gcodes/%2%"
				, get_base_url()
				, Network::IHttp::escape_string(filename));
	} else {
		return format("%1%rr_upload?name=0:/gcodes/%2%&%3%"
				, get_base_url()
				, Network::IHttp::escape_string(filename)
				, timestamp_str());
	}
}

std::string PrintHostDuet::get_connect_url(const bool dsfUrl) const
{
	if (dsfUrl)	{
		return format("%1%machine/connect?password=%2%"
			, get_base_url()
			, (password.empty() ? "reprap" : Network::IHttp::escape_string(password)));
	} else {
		return format("%1%rr_connect?password=%2%&%3%"
				, get_base_url()
				, (password.empty() ? "reprap" : Network::IHttp::escape_string(password))
				, timestamp_str());
	}
}

std::string PrintHostDuet::get_base_url() const
{
	if (host.find("http://") == 0 || host.find("https://") == 0) {
		if (host.back() == '/') {
			return host;
		} else {
			return format("%1%/", host);
		}
	} else {
		return format("http://%1%/", host);
	}
}

std::string PrintHostDuet::timestamp_str() const
{
	enum { BUFFER_SIZE = 32 };

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);

	char buffer[BUFFER_SIZE];
	std::strftime(buffer, BUFFER_SIZE, "time=%Y-%m-%dT%H:%M:%S", &tm);

	return std::string(buffer);
}

bool PrintHostDuet::start_print(std::string &msg, const std::string &filename, PrintHostDuet::ConnectionType connectionType, bool simulationMode, RetryFn retry_fn) const
{
    assert(connectionType != ConnectionType::error);

	bool res = false;
	bool dsf = (connectionType == ConnectionType::dsf);

	auto url = dsf
		? format("%1%machine/code", get_base_url())
		: format(simulationMode
				? "%1%rr_gcode?gcode=M37%%20P\"0:/gcodes/%2%\""
				: "%1%rr_gcode?gcode=M32%%20\"0:/gcodes/%2%\""
			, get_base_url()
			, Network::IHttp::escape_string(filename));
    
    std::unique_ptr<Network::IHttp> http = (dsf 
        ? Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn)
        : Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn));
	if (dsf) {
		http->set_post_body(
				(format(simulationMode
						? "M37 P\"0:/gcodes/%1%\""
						: "M32 \"0:/gcodes/%1%\""
					, filename))
				);
	}
	http->on_error([&](std::string body, std::string error, unsigned status) {
			SPDLOG_ERROR(format("Duet: Error starting print: %1%, HTTP %2%, body: `%3%`", error , status , body));
			msg = format_error(body, error, status);
		})
		.on_complete([&](std::string body, unsigned) {
			SPDLOG_INFO(format("Duet: Got: %1%", body));
			res = true;
		})
		.perform_sync();

	return res;
}

int PrintHostDuet::get_err_code_from_body(const std::string& body) const {
    try {
        nlohmann::json json = nlohmann::json::parse(body);
        return json.value("err", 0);
    } catch (const std::exception& e) {
        SPDLOG_ERROR(format("JSON parsing error: %1%", e.what()));
        return 0;
    }
}

} // namespace Slic3r::Biz::PrintHost