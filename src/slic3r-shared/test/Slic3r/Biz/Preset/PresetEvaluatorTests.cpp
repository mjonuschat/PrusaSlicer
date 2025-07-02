#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"
#include "Slic3r/TestUtils/TestData.hpp"


TEST_CASE("Preset Evaluator", "[preset]")
{
    using namespace Slic3r::Domain::Preset;
    using namespace Slic3r::Biz::Preset;
    using namespace Slic3r::Biz::Preset::IO;
    namespace Yaml = Slic3r::Biz::Yaml;

    PresetLoader loader;
    try {

        const std::string path = Tests::get_datadir().string() + "/preset/prusa-research-fff";
        loader.load_dir(path);

    } catch (const Yaml::ParseError& e) {
        std::cout << e.what() << std::endl;
        FAIL();
    }

    PresetEvaluator eval(loader.presets());

    HwPrinterConfig hw_config = {
        .technology = Slic3r::Domain::PrinterTechnology::FFF,
        .model = {.model = "COREONE", .base_model = "COREONE"},
        .tool_count = 1,
        .features = {},
        .tools = {
            {
                .features = {
                    std::make_pair("nozzle_diameter", FeatureValue{0.4}),
                    std::make_pair("nozzle_high_flow", FeatureValue{false}),
                }
            }
        }
    };

    auto printer_preset = eval.evaluate(hw_config);
    //REQUIRE(printer_preset.preset.values.empty() == false);
    auto values = std::get<Slic3r::Domain::PrinterSettings>(printer_preset.preset.values);
    REQUIRE(values.contains("single_extruder_multi_material").item->value().get<bool>() == false);

    REQUIRE(printer_preset.prints.empty() == false);

    for (const auto& print_preset : printer_preset.prints) {
        REQUIRE(print_preset.tools.size() == hw_config.tool_count);
        for (const auto& tool : print_preset.tools) {
            REQUIRE(tool.empty() == false);
        }
    }

    REQUIRE(printer_preset.materials.empty() == false);

}
