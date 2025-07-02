#pragma once

#include "Slic3r/Biz/Parser/IO.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/FullConfigSLA.hpp"

namespace Slic3r::Biz::Slicing {
Parser::IO::Config get_parser_config(const Domain::FullConfigFDM& config_pack);
Parser::IO::Config get_parser_config(const Domain::FullConfigSLA& config_pack);
}
