#pragma once

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Config.hpp"

namespace Tests {
    inline boost::filesystem::path get_datadir() {
        namespace fs = boost::filesystem;
        return fs::absolute(fs::canonical(fs::path{TEST_DATA_DIR}));
    }

    inline std::pair<Slic3r::Model, Slic3r::DynamicPrintConfig> load_3mf(const boost::filesystem::path &path) {
        using namespace Slic3r;
        REQUIRE(boost::filesystem::exists(path));

        DynamicPrintConfig config;
        Model model;

        ConfigSubstitutionContext context{ForwardCompatibilitySubstitutionRule::Disable};
        boost::optional<Semver> version;
        Slic3r::load_3mf(path.string().c_str(), config, context, &model, false, version);

        return {std::move(model), std::move(config)};
    }
}
