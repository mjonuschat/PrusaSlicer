#include <catch2/catch_test_macros.hpp>

#include "boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp"

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;
using Test::TestConfig;
using Test::TestMesh;

namespace {

std::string slice_cube_with_seam_type(NipTuckSeamType seam_type)
{
    TestConfig config;
    config.print.items.opt("seam_type").set(seam_type);
    config.print.items.opt("seam_position").set(Domain::SeamPosition::spNearest);
    config.print.items.opt("perimeters").set(3);

    Print print;
    Domain::Model model;
    Test::init_print({TestMesh::cube_20x20x20}, print, model, config);
    return Test::gcode(print);
}

} // namespace

TEST_CASE("Nip/Tuck seam_type changes the emitted G-code relative to regular", "[boss][seam][NipTuckSeam]")
{
    const std::string regular_gcode = slice_cube_with_seam_type(NipTuckSeamType::Regular);
    const std::string niptuck_gcode = slice_cube_with_seam_type(NipTuckSeamType::NipTuck);

    // Every export appends the full config as "; option = value" comment lines,
    // including "; seam_type = ...", so that alone would make the two G-codes
    // differ even if modify_perimeters() did nothing. Compare only the moves.
    const std::string config_marker = "; prusaslicer_config = begin";
    const std::string regular_moves = regular_gcode.substr(0, regular_gcode.find(config_marker));
    const std::string niptuck_moves = niptuck_gcode.substr(0, niptuck_gcode.find(config_marker));

    CHECK(regular_moves != niptuck_moves);
}
