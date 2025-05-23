#pragma once

#include "Slic3r/Domain/ConfigFDM.hpp"
#include "Slic3r/Biz/Parser/IO.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Biz::Slicing {
Parser::IO::Config get_parser_config(const Domain::ConfigPackFDM& config_pack);
Domain::FullConfigFDM get_full_config(const Domain::ConfigPackFDM& config_pack);
PrintConfigView get_view(const Domain::ConfigPackFDM& config_pack);
}
