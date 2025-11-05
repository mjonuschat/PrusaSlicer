#pragma once

#include <boost/filesystem/path.hpp>
#include <string>

namespace Slic3r::Biz::FileDownloader {

struct FileDownloaderJobProgressPayload
{
    size_t download_id;
    std::string filename;
    std::string project_url;
    size_t load_count;
    boost::filesystem::path final_path;
};

struct FileDownloaderJobInput
{
    std::string file_name;
    std::string file_url;
    std::string project_name;
    std::string project_url;
    std::string image_url;
    size_t load_count {0};
    bool new_project {false};
};

struct FileDownloaderJobData
{
    // Input
    FileDownloaderJobInput input_data;
    const size_t id;
    boost::filesystem::path dest_folder;

    // Runtime
    boost::filesystem::path tmp_path;
    size_t written{0};
    size_t absolute_size{0};
    bool stopped{false};
    bool paused{false};
    bool finished{false};
};

} // namespace Slic3r::Biz::FileDownloader