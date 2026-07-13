#include "CLITestUtils.hpp"

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Domain/Preset/Bundle.hpp"
#include "Slic3r/Domain/Workbench.hpp"

#include <ranges>
#include <stdexcept>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

using namespace Slic3r;
using namespace Slic3r::Biz;

using Slic3r::Biz::Preset::PresetInteractor;
using Slic3r::Biz::Scene::SceneInteractor;
using Slic3r::Domain::PrinterTechnology;
using Slic3r::Domain::TriangleMesh;
using Slic3r::Domain::Workbench;
using Slic3r::Domain::Preset::Bundle;
using Slic3r::Domain::Preset::EvaluatedMaterialPreset;
using Slic3r::Domain::Preset::EvaluatedPrinterPreset;
using Slic3r::Domain::Preset::EvaluatedPrintPreset;
using Slic3r::Domain::Preset::SingleToolEvaluatedMaterialPresets;

namespace Slic3r::App::CLI::Test {

namespace {

TestProfileSet pick_profile_set(const Bundle& preset_bundle, const bool multi_tool)
{
    for (const std::vector<EvaluatedPrinterPreset>& printer_presets :
         preset_bundle.evaluated_presets | std::views::values)
    {
        for (const EvaluatedPrinterPreset& printer_preset : printer_presets) {
            if (printer_preset.technology() != PrinterTechnology::FFF) {
                continue;
            }

            const std::size_t tool_count = printer_preset.hw_config.tool_count;
            if (multi_tool ? tool_count < 2 : tool_count != 1) {
                continue;
            }

            if (printer_preset.prints.empty()) {
                continue;
            }

            // Prefer a 0.20mm print profile.
            const EvaluatedPrintPreset* chosen_print_preset = &printer_preset.prints.front();
            for (const EvaluatedPrintPreset& print_preset : printer_preset.prints) {
                if (print_preset.preset.name.find("0.20mm") != std::string::npos) {
                    chosen_print_preset = &print_preset;
                    break;
                }
            }

            if (chosen_print_preset->materials.empty()
                || chosen_print_preset->materials.front().empty())
            {
                continue;
            }

            // Prefer a Prusament PLA material.
            const SingleToolEvaluatedMaterialPresets& slot_materials =
                chosen_print_preset->materials.front();
            const EvaluatedMaterialPreset* chosen_material_preset = &slot_materials.front();
            for (const EvaluatedMaterialPreset& material_preset : slot_materials) {
                if (material_preset.preset.name.find("Prusament PLA") != std::string::npos) {
                    chosen_material_preset = &material_preset;
                    break;
                }
            }

            return TestProfileSet{
                .printer_profile_name  = printer_preset.hw_config.name,
                .print_profile_name    = chosen_print_preset->preset.name,
                .material_profile_name = chosen_material_preset->preset.name,
                .tool_count            = tool_count
            };
        }
    }

    throw std::runtime_error(
        multi_tool ? "No suitable multi-tool FFF printer was found in the preset bundle." :
                     "No suitable single-tool FFF printer was found in the preset bundle."
    );
}

const std::pair<TestProfileSet, TestProfileSet>& resolved_profile_sets()
{
    static const std::pair<TestProfileSet, TestProfileSet> profile_sets = []()
    {
        Workbench workbench;
        SceneInteractor scene_interactor{workbench};
        PresetInteractor preset_interactor(workbench, scene_interactor);
        preset_interactor.load_preset_bundle(Preset::IO::BundlePaths::make_standard_runtime());

        const Bundle& preset_bundle = workbench.preset_bundle();
        return std::make_pair(
            pick_profile_set(preset_bundle, false),
            pick_profile_set(preset_bundle, true)
        );
    }();

    return profile_sets;
}

} // namespace

const TestProfileSet& single_tool_profile_set()
{
    return resolved_profile_sets().first;
}

const TestProfileSet& multi_tool_profile_set()
{
    return resolved_profile_sets().second;
}

ScopedTempDir::ScopedTempDir() :
    m_path(
        boost::filesystem::temp_directory_path()
        / boost::filesystem::unique_path("slic3r-cli-test-%%%%-%%%%")
    )
{
    boost::filesystem::create_directories(m_path);
}

ScopedTempDir::~ScopedTempDir()
{
    boost::system::error_code cleanup_error;
    boost::filesystem::remove_all(m_path, cleanup_error);
}

boost::filesystem::path write_cube_stl(
    const boost::filesystem::path& directory,
    const std::string& file_name,
    const double cube_size_mm
)
{
    const boost::filesystem::path stl_path = directory / file_name;
    const TriangleMesh cube_mesh =
        Algorithms::TriangleMesh::make_cube(cube_size_mm, cube_size_mm, cube_size_mm);
    store_stl(stl_path.string(), cube_mesh, true);
    return stl_path;
}

InitParams make_single_tool_params()
{
    const TestProfileSet& profile_set = single_tool_profile_set();

    InitParams init_params;
    init_params.input.printer_profile_preset = profile_set.printer_profile_name;
    init_params.input.print_profile_preset   = profile_set.print_profile_name;
    init_params.input.material_profile_presets.emplace_back(profile_set.material_profile_name);
    return init_params;
}

InitParams make_multi_tool_params()
{
    const TestProfileSet& profile_set = multi_tool_profile_set();

    InitParams init_params;
    init_params.input.printer_profile_preset = profile_set.printer_profile_name;
    init_params.input.print_profile_preset   = profile_set.print_profile_name;
    init_params.input.material_profile_presets.emplace_back(profile_set.material_profile_name);
    return init_params;
}

std::size_t count_gcode_files(const boost::filesystem::path& directory)
{
    std::size_t gcode_file_count = 0;
    for (const boost::filesystem::directory_entry& directory_entry :
         boost::filesystem::directory_iterator(directory))
    {
        if (!boost::filesystem::is_regular_file(directory_entry.path())) {
            continue;
        }

        const std::string extension = directory_entry.path().extension().string();
        if (boost::iequals(extension, ".gcode") || boost::iequals(extension, ".bgcode")) {
            ++gcode_file_count;
        }
    }

    return gcode_file_count;
}

} // namespace Slic3r::App::CLI::Test
