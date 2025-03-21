#include <catch2/catch_test_macros.hpp>
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Preset/Loader/HwConfigLoader.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"

TEST_CASE("Load HW Config")
{
    using namespace Slic3r::Domain;
    using namespace Slic3r::Biz::Preset;
    const std::string filename = Tests::get_datadir().string() + "/preset/hw-config.yaml";
    Loader::HwConfigLoader loader;
    try {
        loader.load(filename);
    } catch (const Yaml::ParseError & e) {
        std::cout << e.what() << std::endl;
        FAIL_CHECK(e.what());
    }
    auto& result = loader.result();
    auto& fff_hw_defs = result[PrinterTechnology::FFF];
    REQUIRE(fff_hw_defs.technology == PrinterTechnology::FFF);
    REQUIRE(fff_hw_defs.printers.size() == 2);
    REQUIRE(fff_hw_defs.tools.size() == 2);
    REQUIRE(fff_hw_defs.feeders.size() == 1);
}