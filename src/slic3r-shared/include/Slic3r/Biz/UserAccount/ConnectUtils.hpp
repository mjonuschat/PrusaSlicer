#pragma once

#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"
namespace Slic3r::Biz::UserAccount::ConnectUtils {

bool config_from_json(const std::string& json, PrintHost::PrintHostConfig& config, std::string& filename, std::string& body_json);

} //namespace Slic3r::Biz::UserAccount