#pragma once

#include "Slic3r/Assert.hpp"
#include <string>
#include <boost/filesystem.hpp>
#include <memory>
#include <variant>

namespace Slic3r::Biz::libpgcode {
    class LineView;
}
namespace Slic3r::Biz::PrintHost {

using DataPtrVariant = std::variant<std::shared_ptr<const libpgcode::LineView>>;

enum class PrintHostType {
    Local,
    PrusaLink,
    PrusaLinkStorage,
    PrusaConnect,
    SL1Host,
    OctoPrint,
    Moonraker,
    Duet,
    FlashAir,
    AstroBox,
    Repetier,
    MKS,
};

enum class PrintHostAfterUploadAction {
    None,
    StartPrint,
    StartSimulation,
    QueuePrint
};

enum class PrintHostAuthType {
    None,
    ApiKey,
    Digest
};


struct PrintHostConfig
{
    PrintHostType type;
    std::string host;
    std::string api_key;
    std::string username;
    std::string password;
    std::string ca_file;
    bool ssl_revoke_best_effort;
    PrintHostAuthType auth_type;
    std::string port;
    std::string team_id;
    std::string printer_uuid;
    std::string access_token;

    PrintHostConfig() = delete;

    PrintHostConfig(PrintHostType type, 
        std::string host, 
        std::string ca_file = std::string(), 
        bool ssl_revoke_best_effort = false,  
        std::string port = std::string(),
        std::string team_id = std::string(),
        std::string printer_uuid = std::string(),
        std::string access_token = std::string())
        : type(type)
        , host(std::move(host))
        , ca_file(std::move(ca_file))
        , ssl_revoke_best_effort(ssl_revoke_best_effort)
        , auth_type(PrintHostAuthType::None)
        , port(std::move(port))
        , team_id(std::move(team_id))
        , printer_uuid(std::move(printer_uuid))
        , access_token(std::move(access_token))
    {}

    PrintHostConfig(PrintHostType type, 
        std::string host, 
        std::string api_key, 
        std::string ca_file = std::string(), 
        bool ssl_revoke_best_effort = false,
        std::string port = std::string(), 
        std::string team_id = std::string(),
        std::string printer_uuid = std::string(),
        std::string access_token = std::string())
        : type(type)
        , host(std::move(host))
        , api_key(std::move(api_key))
        , ca_file(std::move(ca_file))
        , ssl_revoke_best_effort(ssl_revoke_best_effort)
        , auth_type(PrintHostAuthType::ApiKey)
        , port(std::move(port))
        , team_id(std::move(team_id))
        , printer_uuid(std::move(printer_uuid))
        , access_token(std::move(access_token))
    {}
    
    PrintHostConfig(PrintHostType type, 
        std::string host, 
        std::string username, 
        std::string password, 
        std::string ca_file = std::string(),          
        bool ssl_revoke_best_effort = false,
        std::string port = std::string(),
        std::string team_id = std::string(),
        std::string printer_uuid = std::string(),
        std::string access_token = std::string())
        : type(type)
        , host(std::move(host))
        , username(std::move(username))
        , password(std::move(password))
        , ca_file(std::move(ca_file))
        , ssl_revoke_best_effort(ssl_revoke_best_effort)
        , auth_type(PrintHostAuthType::Digest)
        , port(std::move(port))
        , team_id(std::move(team_id))
        , printer_uuid(std::move(printer_uuid))
        , access_token(std::move(access_token))
    {}

    PrintHostConfig(PrintHostConfig&& other) noexcept
        : type(other.type),
          host(std::move(other.host)),
          api_key(std::move(other.api_key)),
          username(std::move(other.username)),
          password(std::move(other.password)),
          ca_file(std::move(other.ca_file)),
          ssl_revoke_best_effort(other.ssl_revoke_best_effort),
          auth_type(other.auth_type),
          port(std::move(other.port)),
          team_id(std::move(other.team_id)),
          printer_uuid(std::move(other.printer_uuid)),
          access_token(std::move(other.access_token))
    {}

    PrintHostConfig& operator=(PrintHostConfig&& other) noexcept
    {
        if (this != &other) {
            type = other.type;
            host = std::move(other.host);
            api_key = std::move(other.api_key);
            username= std::move(other.username);
            password= std::move(other.password);
            ca_file = std::move(other.ca_file);
            port = std::move(other.port);
            ssl_revoke_best_effort = other.ssl_revoke_best_effort;
            auth_type = other.auth_type;
            team_id = std::move(other.team_id);
            printer_uuid = std::move(other.printer_uuid);
            access_token = std::move(other.access_token);
        }
        return *this;
    }

    PrintHostConfig(const PrintHostConfig& other) = delete;
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

    PrintHostAfterUploadAction post_action;
    std::string storage;
    std::string group;
    std::string request_body_json;

    PrintHostExportFormat result_format;

    PrintHostJobData() = delete;

    PrintHostJobData(
        DataPtrVariant data, 
        const boost::filesystem::path& dest,
        PrintHostExportFormat result_format,
        PrintHostAfterUploadAction action = PrintHostAfterUploadAction::None,
        std::string storage = std::string(),
        std::string group = std::string(),
        std::string request_body_json = std::string()
    )
        : data_ptr(data)
        , dest_path(dest)
        , post_action(action)
        , storage(std::move(storage))
        , group(std::move(group))
        , request_body_json(std::move(request_body_json))
        , result_format(result_format)
    {}


    PrintHostJobData(PrintHostJobData&& other) noexcept
        : data_ptr(std::move(other.data_ptr))
        , source_path(std::move(other.source_path))
        , dest_path(std::move(other.dest_path))
        , post_action(other.post_action)
        , storage(std::move(other.storage))
        , group(std::move(other.group))
        , request_body_json(std::move(other.request_body_json))
        , result_format(other.result_format)
    {}

    PrintHostJobData& operator=(PrintHostJobData&& other) noexcept
    {
        if (this != &other) {
            data_ptr = std::move(other.data_ptr);
            source_path = std::move(other.source_path);
            dest_path = std::move(other.dest_path);
            post_action = other.post_action;
            storage = std::move(other.storage);
            group = std::move(other.group);
            request_body_json = std::move(other.request_body_json);
            result_format = other.result_format;
        }
        return *this;
    }

    PrintHostJobData(const PrintHostJobData& other) = delete;
    PrintHostJobData& operator=(const PrintHostJobData& other) = delete;
};

struct PrintHostStorageInfo
{
    std::string name;
    std::string path;
    bool read_only = false;
    long long free_space = -1;
};

} // Slic3r::Biz::PrintHost