#include <catch2/catch_test_macros.hpp>
#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"
#include "Slic3r/TestUtils/TestData.hpp"

#include <mutex>

TEST_CASE("PresetLoader preset-filament-common.yaml", "[preset]")
{
    using namespace Slic3r::Biz::Preset::IO;
    using namespace Slic3r::Domain::Preset;
    namespace Yaml = Slic3r::Biz::Yaml;

    PresetLoader loader;
    try {
        for (auto filename : std::array{"preset-filament-common.yaml", "preset-filament-prusament-pla.yaml"}) {
            const std::string path = Tests::get_datadir().string() + "/presets/" + filename;
            std::mutex mutex;
            loader.load(path, mutex);
        }
    } catch (const Yaml::ParseError& e) {
        std::cout << e.what() << std::endl;
        FAIL();
    }

    const auto& presets = loader.presets();
    REQUIRE(presets.size() == 1);
    auto filament_presets_it = presets.find(PresetKind::FdmMaterial);
    REQUIRE(filament_presets_it != presets.end());
    const auto& filament_presets = filament_presets_it->second;
    REQUIRE(filament_presets.size() == 4);

    auto pla_preset_it = std::find_if(filament_presets.begin(), filament_presets.end(), [](const auto& preset) { return preset.id == "*PLA*"; });
    REQUIRE(pla_preset_it != filament_presets.end());
    const auto& pla_preset = *pla_preset_it;
    REQUIRE(pla_preset.kind == PresetKind::FdmMaterial);
    REQUIRE_FALSE(pla_preset.name.has_value());
    REQUIRE(pla_preset.inherits.size() == 0);
    REQUIRE(pla_preset.values.size() == 39);
    REQUIRE(pla_preset.variants.size() == 2);


    const auto& pla_v0 = pla_preset.variants[0];
    REQUIRE(boost::get<Slic3r::Domain::Expr::VarRef>(*pla_v0.condition.value()).name == "printer.planetary_gearbox");

    auto fcfs_it = pla_v0.values.find("filament_cooling_final_speed");
    REQUIRE((fcfs_it != pla_v0.values.end()));
    REQUIRE(std::holds_alternative<double>(fcfs_it->second));

    auto sfg_it = pla_v0.values.find("start_filament_gcode");
    REQUIRE(sfg_it != pla_v0.values.end());
    REQUIRE(std::holds_alternative<std::string>(sfg_it->second));

}
