#include <catch2/catch_test_macros.hpp>

#include "CLITestUtils.hpp"

#include "Slic3r/App/CLI/CLIApp.hpp"
#include "Slic3r/App/Init.hpp"
#include "Slic3r/Domain/Percentage.hpp"

#include <cstdlib>

#include <boost/filesystem.hpp>

using Slic3r::Domain::FloatOrPercentage;

using namespace Slic3r;
using namespace Slic3r::App::CLI::Test;

namespace fs = boost::filesystem;

TEST_CASE("CLI slices a single-tool 3MF project", "[cli][timeout]")
{
    const ScopedTempDir temp_dir;
    const fs::path cube_stl_path     = write_cube_stl(temp_dir.path(), "cube.stl", 20.);
    const fs::path exported_3mf_path = temp_dir.path() / "cube.3mf";
    const fs::path gcode_path        = temp_dir.path() / "print.gcode";

    App::InitParams export_params = make_single_tool_params();
    export_params.input.input_files.push_back(cube_stl_path.string());
    export_params.action.export_3mf = true;
    export_params.misc.output       = exported_3mf_path.string();

    REQUIRE(App::CLI::run(export_params) == EXIT_SUCCESS);
    REQUIRE(fs::exists(exported_3mf_path));

    App::InitParams slice_params;
    slice_params.input.input_files.push_back(exported_3mf_path.string());
    slice_params.action.export_gcode = true;
    slice_params.misc.output         = gcode_path.string();

    REQUIRE(App::CLI::run(slice_params) == EXIT_SUCCESS);
    REQUIRE(fs::exists(gcode_path));
    REQUIRE(fs::file_size(gcode_path) > 0);
}

TEST_CASE("CLI slices a multi-tool 3MF project", "[cli][timeout]")
{
    const ScopedTempDir temp_dir;
    const fs::path cube_stl_path     = write_cube_stl(temp_dir.path(), "cube.stl", 20.);
    const fs::path exported_3mf_path = temp_dir.path() / "cube.3mf";
    const fs::path gcode_path        = temp_dir.path() / "print.gcode";

    App::InitParams export_params = make_multi_tool_params();
    export_params.input.input_files.push_back(cube_stl_path.string());
    export_params.action.export_3mf = true;
    export_params.misc.output       = exported_3mf_path.string();

    REQUIRE(App::CLI::run(export_params) == EXIT_SUCCESS);
    REQUIRE(fs::exists(exported_3mf_path));

    App::InitParams slice_params;
    slice_params.input.input_files.push_back(exported_3mf_path.string());
    slice_params.action.export_gcode = true;
    slice_params.misc.output         = gcode_path.string();

    REQUIRE(App::CLI::run(slice_params) == EXIT_SUCCESS);
    REQUIRE(fs::exists(gcode_path));
    REQUIRE(fs::file_size(gcode_path) > 0);
}

TEST_CASE("CLI slices multiple STL inputs into separate G-codes", "[cli][timeout]")
{
    const ScopedTempDir temp_dir;
    const fs::path first_cube_stl_path  = write_cube_stl(temp_dir.path(), "cube_a.stl", 20.);
    const fs::path second_cube_stl_path = write_cube_stl(temp_dir.path(), "cube_b.stl", 20.);

    App::InitParams slice_params = make_single_tool_params();
    slice_params.input.input_files.push_back(first_cube_stl_path.string());
    slice_params.input.input_files.push_back(second_cube_stl_path.string());
    slice_params.action.export_gcode = true;
    slice_params.misc.output         = temp_dir.path().string();

    REQUIRE(App::CLI::run(slice_params) == EXIT_SUCCESS);
    REQUIRE(count_gcode_files(temp_dir.path()) == 2);
}

TEST_CASE("CLI reports objects scaled outside the print volume instead of freezing", "[cli]")
{
    const ScopedTempDir temp_dir;
    const fs::path cube_stl_path = write_cube_stl(temp_dir.path(), "cube.stl", 20.);
    const fs::path gcode_path    = temp_dir.path() / "huge.gcode";

    App::InitParams slice_params = make_single_tool_params();
    slice_params.input.input_files.push_back(cube_stl_path.string());
    slice_params.transform.scale     = FloatOrPercentage{100.0};
    slice_params.action.export_gcode = true;
    slice_params.misc.output         = gcode_path.string();

    REQUIRE(App::CLI::run(slice_params) == EXIT_SUCCESS);
    REQUIRE_FALSE(fs::exists(gcode_path));
    REQUIRE(count_gcode_files(temp_dir.path()) == 0);
}
