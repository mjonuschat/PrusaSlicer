// Proves the config-path SCV->JD conversion in TimeProcessor::update_machine_accelerations
// actually reaches the time estimator through a real slice, not just that
// KlipperCorneringModel's pure math is correct in isolation. This does not
// exercise ProcessorImpl::process_SET_VELOCITY_LIMIT -- see
// test_boss_klipper_cornering.cpp in tests/libslic3r for that.
#include <catch2/catch_test_macros.hpp>

#include <variant>

#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

float estimated_normal_time(const TestConfig &config)
{
    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
    const Biz::libpgcode::ProcessorResult result{print.process_gcode()};
    return std::visit([](const auto &stats) { return stats.normal_mode_time.time; }, result.print_statistics);
}

} // namespace

TEST_CASE("Klipper machine_max_jerk_x/_y changes the estimated print time", "[boss][klipper]")
{
    TestConfig low_jerk_config;
    low_jerk_config.printer.items.opt("gcode_flavor").set(Domain::GCodeFlavor::gcfKlipper);
    low_jerk_config.printer.items.opt("machine_max_jerk_x").set(std::vector<double>{1.0});
    low_jerk_config.printer.items.opt("machine_max_jerk_y").set(std::vector<double>{1.0});

    TestConfig high_jerk_config;
    high_jerk_config.printer.items.opt("gcode_flavor").set(Domain::GCodeFlavor::gcfKlipper);
    high_jerk_config.printer.items.opt("machine_max_jerk_x").set(std::vector<double>{20.0});
    high_jerk_config.printer.items.opt("machine_max_jerk_y").set(std::vector<double>{20.0});

    float low_jerk_time = estimated_normal_time(low_jerk_config);
    float high_jerk_time = estimated_normal_time(high_jerk_config);

    REQUIRE(low_jerk_time > 0.f);
    REQUIRE(high_jerk_time > 0.f);
    // Lower SCV -> lower JD -> slower cornering at every perimeter corner.
    REQUIRE(low_jerk_time > high_jerk_time);
}
