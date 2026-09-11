#include <catch2/catch_test_macros.hpp>

#include <libslic3r/Point.hpp>

#include "Slic3r/Biz/GCodeReader/GCodeReader.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using Biz::GCodeReader::GCodeReader;

namespace {

double average_seam_y(const std::string &gcode)
{
    bool   was_extruding = false;
    Points seam_points;
    GCodeReader parser;
    parser.parse_buffer(gcode, [&was_extruding, &seam_points](
                                    GCodeReader &self, const GCodeReader::GCodeLine &line) {
        if (line.extruding(self)) {
            if (!was_extruding)
                seam_points.emplace_back(self.xy_scaled());
            was_extruding = true;
        } else if (!line.cmd_is("M73")) {
            was_extruding = false;
        }
    });

    REQUIRE(!seam_points.empty());

    double sum_y = 0.0;
    for (const Point &point : seam_points)
        sum_y += unscaled<double>(point.y());
    return sum_y / static_cast<double>(seam_points.size());
}

std::string slice_aligned_rear(bool aligned_rear)
{
    Test::TestConfig config;
    config.print.items.opt("seam_position").set(Domain::SeamPosition::spAligned);
    config.print.items.opt("seam_position_aligned_rear").set(aligned_rear);
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{0.0});
    config.print.items.opt("top_solid_layers").set(0);
    config.print.items.opt("bottom_solid_layers").set(0);
    config.print.items.opt("retract_layer_change").set(false);

    return Test::slice({Test::TestMesh::V}, config);
}

} // namespace

TEST_CASE("Enabling aligned-rear shifts the emitted seam toward the model's back", "[boss][seam]")
{
    const double average_y_off = average_seam_y(slice_aligned_rear(false));
    const double average_y_on  = average_seam_y(slice_aligned_rear(true));

    // Rear is +Y (Rear::get_max_y_choice picks the max-Y corner), matching
    // the bias formula's penalty on -Y-facing (front) surface normals.
    CHECK(average_y_on > average_y_off);
}
