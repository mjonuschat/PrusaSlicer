#pragma once

#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"
#include <string>
#include <boost/filesystem.hpp>
#include <memory>
#include <variant>

#include "Slic3r/Domain/ConfigPhysical.hpp"

namespace Slic3r::Biz::libpgcode {
class LineView;
} // namespace Slic3r::Biz::libpgcode

namespace Slic3r::Biz::PrintHost {

using DataPtrVariant = std::variant<std::shared_ptr<const libpgcode::LineView>>;

enum class PrintHostAfterUploadAction
{
    None,
    StartPrint,
    StartSimulation,
    QueuePrint
};

struct PrintHostConfig
{
    Domain::PrintHostType type;
    std::string host;
    std::string api_key;
    std::string username;
    std::string password;
    std::string ca_file;
    bool ssl_revoke_best_effort{false};
    Domain::PrintHostAuthType auth_type{Domain::PrintHostAuthType::None};
    std::string port;
    std::string team_id;
    std::string printer_uuid;
    std::string access_token;

    PrintHostConfig() = delete;

    PrintHostConfig(Domain::PrintHostType type, std::string host) :
        type(type),
        host(std::move(host))
    {}

    PrintHostConfig(Domain::PrintHostType type, std::string host, std::string api_key) :
        type(type),
        host(std::move(host)),
        api_key(std::move(api_key)),
        auth_type(Domain::PrintHostAuthType::ApiKey)
    {}

    PrintHostConfig(Domain::PrintHostType type, std::string host, std::string username, std::string password) :
        type(type),
        host(std::move(host)),
        username(std::move(username)),
        password(std::move(password)),
        auth_type(Domain::PrintHostAuthType::Digest)
    {}

    PrintHostConfig(PrintHostConfig&& other) noexcept            = default;
    PrintHostConfig& operator=(PrintHostConfig&& other) noexcept = default;

    PrintHostConfig(const PrintHostConfig& other)            = delete;
    PrintHostConfig& operator=(const PrintHostConfig& other) = delete;
};

enum class PrintHostExportFormat
{
    Undefined,
    GCode,
    BGCode,
};

inline PrintHostExportFormat get_export_format_from_extension(const std::string& extension)
{
    SPDLOG_INFO("{} extension: {}", __FUNCTION__, extension);
    if (extension == ".gcode")
        return PrintHostExportFormat::GCode;
    if (extension == ".bgcode")
        return PrintHostExportFormat::BGCode;
    ASSERT(false, "Unknown data format. Add it to PrintHostResultFormat");
    return PrintHostExportFormat::Undefined;
}

struct PrintHostJobData
{
    DataPtrVariant data_ptr;

    boost::filesystem::path source_path;
    boost::filesystem::path dest_path;

    PrintHostAfterUploadAction post_action{PrintHostAfterUploadAction::None};
    std::string storage;
    std::string group;
    std::string request_body_json;

    PrintHostExportFormat result_format;

    PrintHostJobData() = delete;

    PrintHostJobData(
        DataPtrVariant data,
        const boost::filesystem::path& dest,
        PrintHostExportFormat result_format
    ) :
        data_ptr(data),
        dest_path(dest),
        result_format(result_format)
    {}

    PrintHostJobData(PrintHostJobData&& other) noexcept = default;

    PrintHostJobData& operator=(PrintHostJobData&& other) noexcept = default;

    PrintHostJobData(const PrintHostJobData& other)            = delete;
    PrintHostJobData& operator=(const PrintHostJobData& other) = delete;
};

struct PrintHostStorageInfo
{
    std::string name;
    std::string path;
    bool read_only       = false;
    long long free_space = -1;
};

} // namespace Slic3r::Biz::PrintHost
