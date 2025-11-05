#include "Slic3r/Biz/FileDownloader/FileDownloaderInteractor.hpp"

#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Log.hpp"
#include <fmt/format.h>

#include <string_view>

namespace Slic3r::Biz::FileDownloader {

#ifndef NDEBUG
constexpr bool debug{true};
#else
constexpr bool debug{false};
#endif

namespace {
bool is_any_subdomain(const std::string& url, const std::vector<std::string>& subdomains)
{
    for (const std::string& sub : subdomains) {
        if (Network::IHttp::is_subdomain(url, sub))
            return true;
    }
    return false;
}

std::string filename_from_url(const std::string& url)
{
    std::string url_plain = std::string(url.begin(), std::find(url.begin(), url.end(), '?'));
    size_t slash          = url_plain.find_last_of("/");
    if (slash == std::string::npos)
        return std::string();
    return std::string(url_plain.begin() + slash + 1, url_plain.end());
}

} // namespace

void FileDownloaderInteractor::download_files_prusaslicer_url(
    const std::vector<std::string>& files_url
)
{
    for (const std::string& full_url : files_url) {
        std::string escaped_url            = Network::IHttp::unescape_string(full_url);
        constexpr std::string_view prefix1 = "prusaslicer://open?file=";
        constexpr std::string_view prefix2 = "prusaslicer://open/?file=";

        if (escaped_url.starts_with(prefix1)) {
            escaped_url = escaped_url.substr(prefix1.length());
        } else if (escaped_url.starts_with(prefix2)) {
            escaped_url = escaped_url.substr(prefix2.length());
        } else {
            SPDLOG_ERROR("Could not start download due to wrong URL: {}", full_url);
            return;
        }
        FileDownloaderJobInput input;
        input.file_url = escaped_url;
        input.load_count = 0;
        init_download_job(std::move(input));
    }
}

void FileDownloaderInteractor::init_download_job(FileDownloaderJobInput input_data)
{
    
    std::string escaped_url = input_data.file_url;

    // https check
    constexpr std::string_view prefix = "https://";
    if (!escaped_url.starts_with(prefix)) {
        std::string msg = fmt::format(
            "Download won't start. Download URL isn't secure (https) : {}",
            escaped_url
        );
        SPDLOG_ERROR("{}", msg);
        return;
    }

    // subdomain check
    if (!is_any_subdomain(escaped_url, {"printables.com", "thingiverse.com", "cults3d.com"})) {
        std::string msg = fmt::format(
            "Download won't start. Download URL doesn't point to allowed subdomains : {}",
            escaped_url
        );
        SPDLOG_ERROR("{}", msg);
        // TODO: dispatch msg to notification
        return;
    }
    
    if (input_data.file_name.empty()) {
        input_data.file_name = filename_from_url(escaped_url);
    }
    
    boost::filesystem::path dest_directory(system_downloads_dir());
    FileDownloaderJobData data{
        std::move(input_data),
        get_next_id(),
        std::move(dest_directory),
    };

    Platform::JobManager::JobManager& job_manager =
        Platform::PlatformServices::instance().job_manager();
    job_manager.create_job("file_download" + std::to_string(data.id), perform_job, std::move(data))
        .on_result(
            [this](FileDownloaderJobData job_data)
            {
                if (!job_data.finished) {
                    return;
                }
                for (size_t i = 0; i < job_data.input_data.load_count; i++) {
                    boost::filesystem::path model_path = job_data.dest_folder / job_data.input_data.file_name;
                    invoke_listeners<IFileDownloaderListener>(
                        [model_path](auto* listener) { listener->on_model_downloaded(model_path); }
                    );
                }
                
            }
        )
        .on_exception(
            [](const std::exception_ptr& exception, const cpptrace::stacktrace& stacktrace)
            {
                try {
                    std::rethrow_exception(exception);
                } catch (const FileDownloadFailed&) {
                   // No action needed.
                } catch (...) {
                    if (debug) {
                        stacktrace.print();
                    }
                    throw(exception);
                }
            }
        )
        .start();
        
}

} // namespace Slic3r::Biz::FileDownloader
