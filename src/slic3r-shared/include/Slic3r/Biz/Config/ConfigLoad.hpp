#pragma once
#include "Slic3r/Domain/ConfigPack.hpp"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

namespace Slic3r::Biz::Config {

enum ItemParsingIssue {
    InvalidFormat,
    NotFound,
    ExtraKey,
    InvalidEnumValue
};

enum GlobalParsingIssue {
    UnableToDeducePrinterTechnology,
    NotAJsonObject,
    InvalidFDMPrinterSettings,
    InvalidFDMToolSettings,
    InvalidFDMPrintSettings,
    InvalidFDMFilamentSettings,
    InvalidFDMProjectSettings,
    InvalidSLAPrinterSettings,
    InvalidSLAMaterialSettings,
    InvalidSLAPrintSettings,
    FilamentsAndToolsCountIsNotEqual
};


using BoxIssues = std::map<std::string, ItemParsingIssue>;
using LocationIssues = std::variant<BoxIssues, std::vector<BoxIssues>>;
using IssuesPerLocation = std::map<Domain::ConfigLocation, LocationIssues>;

struct LoadResult {
    Domain::ConfigPack config;
    IssuesPerLocation issues;
};

tl::expected<LoadResult, GlobalParsingIssue> load(const nlohmann::ordered_json&);
}
