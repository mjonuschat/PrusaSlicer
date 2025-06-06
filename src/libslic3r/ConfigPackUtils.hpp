#pragma once

#include "Slic3r/Biz/Parser/IO.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Biz::Slicing {
Parser::IO::Config get_parser_config(const Domain::ConfigPack& config_pack);
PrintConfigView get_view(const Domain::ConfigPackFDM& config_pack);
SLAPrintConfigView get_view(const Domain::ConfigPackSLA& config_pack);
}
