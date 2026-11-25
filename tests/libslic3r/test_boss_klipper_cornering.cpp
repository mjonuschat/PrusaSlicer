#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <string>
#include <variant>

#include "Slic3r/Biz/libpgcode/Processor.hpp"
#include "Slic3r/Biz/libpgcode/boss/KlipperCorneringModel.hpp"

using namespace Slic3r::Boss;

TEST_CASE("KlipperCorneringModel::junction_deviation_from_scv matches Klipper's own formula", "[boss][klipper]")
{
    constexpr float kSqrt2Minus1 = 0.41421356237f;
    float scv = 5.0f, max_accel = 500.0f;
    float expected = scv * scv * kSqrt2Minus1 / max_accel;
    CHECK(KlipperCorneringModel::junction_deviation_from_scv(scv, max_accel) == Catch::Approx(expected));
}

TEST_CASE("KlipperCorneringModel::junction_deviation_from_scv is zero when max_accel is non-positive", "[boss][klipper]")
{
    CHECK(KlipperCorneringModel::junction_deviation_from_scv(5.0f, 0.0f) == 0.0f);
    CHECK(KlipperCorneringModel::junction_deviation_from_scv(5.0f, -1.0f) == 0.0f);
}

TEST_CASE("KlipperCorneringModel::centripetal_velocity_limit matches Klipper's inscribed-circle formula", "[boss][klipper]")
{
    float move_distance = 2.0f, accel = 1000.0f, cos_half_theta = 0.8f;
    float sin_half_theta = std::sqrt(1.0f - cos_half_theta * cos_half_theta);
    float tan_half_theta = sin_half_theta / cos_half_theta;
    float expected = std::sqrt(0.5f * move_distance * accel * tan_half_theta);
    CHECK(KlipperCorneringModel::centripetal_velocity_limit(move_distance, accel, cos_half_theta)
          == Catch::Approx(expected));
}

namespace {

using Slic3r::Biz::libpgcode::Processor;
using Slic3r::Biz::libpgcode::ProcessorConfig;
using Slic3r::Biz::libpgcode::ProcessorResult;

std::string square_path_gcode(const std::string &set_velocity_limit_lines)
{
    return "G1 X0 Y0 F3000\n"
           "G1 X10 Y0 F3000\n"
           + set_velocity_limit_lines +
           "G1 X10 Y10 F3000\n"
           "G1 X0 Y10 F3000\n"
           "G1 X0 Y0 F3000\n";
}

float estimated_normal_time(std::string gcode)
{
    ProcessorConfig config;
    config.flavor = Slic3r::Domain::GCodeFlavor::gcfKlipper;
    Processor processor{std::move(config)};
    processor.process_buffer(std::move(gcode));
    ProcessorResult result = processor.finalize();
    return std::visit([](const auto &stats) { return stats.normal_mode_time.time; }, result.print_statistics);
}

} // namespace

TEST_CASE("ProcessorImpl::process_SET_VELOCITY_LIMIT is line-independent between a merged and a split line",
          "[boss][klipper]")
{
    float baseline_time = estimated_normal_time(square_path_gcode(""));
    float merged_time = estimated_normal_time(
        square_path_gcode("SET_VELOCITY_LIMIT ACCEL=1000 SQUARE_CORNER_VELOCITY=5\n"));
    float split_time = estimated_normal_time(
        square_path_gcode("SET_VELOCITY_LIMIT ACCEL=1000\nSET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY=5\n"));

    CHECK(merged_time != Catch::Approx(baseline_time));
    CHECK(merged_time == Catch::Approx(split_time));
}
