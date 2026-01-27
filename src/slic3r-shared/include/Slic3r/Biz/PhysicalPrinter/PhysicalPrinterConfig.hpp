#pragma once

#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Biz/ObservableListWithSelection.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::Biz::PhysicalPrinter {

enum class OperationType
{
    None,
    Local,
    PrusaLinkStorage,
    PrusaConnect,
    PrintHost
};
struct CloudAuth {
    std::string team_id;
    std::string printer_uuid;
    std::string access_token; // needs to be filled before upload
};

struct LocalAuth {
    Domain::PrintHostType type;
    std::string api_key;
    std::string username;
    std::string password;
    std::string ca_file;
    std::string port;
    Domain::PrintHostAuthType auth_type{Domain::PrintHostAuthType::None};
    bool ssl_revoke_best_effort{false};
};

struct PhysicalPrinterConfig {
    OperationType operation_type;
    std::string name;
    std::string uuid;
    std::string host;
    std::variant<std::monostate, CloudAuth, LocalAuth> connection_data;
    std::string base_model;
};

inline PhysicalPrinterConfig local()
{
    return {OperationType::Local};
}

PhysicalPrinterConfig none();

std::string physical_printer_type_to_string(const PhysicalPrinterConfig& data);

using PhysicalPrinterObservableList = ObservableListWithSelection<PhysicalPrinterConfig>;

} //namespace Slic3r::Biz::PhysicalPrinter