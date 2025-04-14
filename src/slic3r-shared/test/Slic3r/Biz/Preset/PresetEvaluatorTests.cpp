#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/Loader/PresetLoader.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"
#include "Slic3r/TestUtils/TestData.hpp"


TEST_CASE("Preset Evaluator")
{
    using namespace Slic3r::Biz::Preset::Loader;
    using namespace Slic3r::Domain::Preset;


    PresetLoader loader;
    try {
        for (auto filename : std::array{"preset-filament-common.yaml", "preset-filament-prusament-pla.yaml"}) {
            const std::string path = Tests::get_datadir().string() + "/preset/" + filename;
            loader.load(path);
        }
    } catch (const Yaml::ParseError& e) {
        std::cout << e.what() << std::endl;
        FAIL();
    }

}