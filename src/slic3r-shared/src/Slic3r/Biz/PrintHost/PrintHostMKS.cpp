#include "Slic3r/Biz/PrintHost/PrintHostMKS.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/Biz/Network/TCPConsole.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

bool PrintHostMKS::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
    bool res = true;

	auto upload_cmd = get_upload_url(upload_data.dest_path.string());
	SPDLOG_INFO(format("MKS: Uploading file. filepath: %1%, print: %2%, command: %3%"
		, upload_data.dest_path
		, (upload_data.post_action == PrintHostAfterUploadAction::StartPrint)
		, upload_cmd));

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(upload_cmd), retry_fn);
	http->set_post_body(std::move(upload_data.raw_data));
	http->on_complete([&](std::string body, unsigned status) {
		SPDLOG_INFO(format("MKS: File uploaded: HTTP %1%: %2%", status , body));
        int err_code;
        try {
            nlohmann::json json = nlohmann::json::parse(body);
            err_code = json.value("err", 0);
		}
		catch (const std::exception&) {
			SPDLOG_INFO(format("Failed to parse MKS on_complete msg json: ", body));
		}

		if (err_code != 0) {
			SPDLOG_INFO(format("MKS: Request completed but error code was received: %1%", err_code));
			error_fn(format_error(body, L("Unknown error occurred"), 0));
			res = false;
		} else if (upload_data.post_action == PrintHostAfterUploadAction::StartPrint) {
			std::string error_msg;
			res = start_print(error_msg, upload_data.dest_path.string());
			if (!res) {
				error_fn(std::move(error_msg));
			}
		}
		})
		.on_error([&](std::string body, std::string error, unsigned status) {
			SPDLOG_ERROR(format("MKS: Error uploading file: %1%, HTTP %2%, body: `%3%`", error , status , body));
			error_fn(format_error(body, error, status));
			res = false;
		})
		.on_progress([&](Network::IHttp::Progress progress, bool& cancel) {
			progress_fn(std::move(progress), cancel);
			if (cancel) {
				// Upload was canceled
				SPDLOG_INFO("MKS: Upload canceled");
				res = false;
			}
		}).perform_sync();


	return res;
}

bool PrintHostMKS::test(std::string& msg, RetryFn retry_fn) const
{
    Network::TCPConsole console(m_print_host_config.host, m_console_port);

	console.enqueue_cmd("M105");
	bool ret = console.run_queue();

	if (!ret)
		msg = console.error_message();

	return ret;
}

std::string PrintHostMKS::get_upload_url(const std::string& filename) const
{
	return format("http://%1%/upload?X-Filename=%2%"
		, m_print_host_config.host
		, Network::IHttp::escape_string(filename));
}

bool PrintHostMKS::start_print(std::string& msg, const std::string& filename) const
{
	// For some reason printer firmware does not want to respond on gcode commands immediately after file upload.
	// So we just introduce artificial delay to workaround it.
	// TODO: Inspect reasons
	std::this_thread::sleep_for(std::chrono::milliseconds(1500));

	Network::TCPConsole console(m_print_host_config.host, m_console_port);

	console.enqueue_cmd(std::string("M23 ") + filename);
	console.enqueue_cmd("M24");

	bool ret = console.run_queue();

	if (!ret)
		msg = console.error_message();

	return ret;
}

} // namespace Slic3r::Biz::PrintHost