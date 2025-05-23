#pragma once

#include "Slic3r/Domain/ConfigFDM.hpp"
#include "Slic3r/Biz/Parser/IO.hpp"
#include "libslic3r/ConfigViews.hpp"

namespace Slic3r::Biz::Slicing {

struct ConfigPackFDM {

    explicit ConfigPackFDM(const int extruder_count)
        : tool{std::vector<Domain::ToolPrintSettings>(extruder_count)}
        , filament{std::vector<Domain::FilamentSettings>(extruder_count)}
    {}
    ConfigPackFDM(): ConfigPackFDM{1} {}

	Domain::PrinterSettings printer;
	Domain::PrintSettings print;
	std::vector<Domain::ToolPrintSettings> tool;
	std::vector<Domain::FilamentSettings> filament;
	Domain::ProjectSettings project;

    Parser::IO::Config get_parser_config() const;
    Domain::FullConfigFDM get_full_config() const;
    PrintConfigView get_view() const;
};
}
