#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Exception.hpp"
#include "libslic3r/ExtrusionRole.hpp"
#include "libslic3r/boss/gcode/flow/SmallAreaFlowCompensator.hpp"

using namespace Slic3r::Boss;

namespace {
SmallAreaFlowCompensator make_default_compensator()
{
    return SmallAreaFlowCompensator(
        {0, 0.2, 0.4, 0.6, 0.8, 1.5, 2, 3, 5, 10},
        {0, 0.4444, 0.6145, 0.7059, 0.7619, 0.8571, 0.8889, 0.9231, 0.9520, 1.0}
    );
}
} // namespace

TEST_CASE("SmallAreaFlowCompensator rejects a non-zero first length", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({1, 2}, {0.5, 1.0}), Slic3r::InvalidArgument);
}

TEST_CASE("SmallAreaFlowCompensator rejects non-increasing lengths", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({0, 1, 1}, {0.5, 0.8, 1.0}), Slic3r::InvalidArgument);
}

TEST_CASE("SmallAreaFlowCompensator rejects non-increasing factors", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({0, 1, 2}, {0.5, 0.5, 1.0}), Slic3r::InvalidArgument);
}

TEST_CASE("SmallAreaFlowCompensator requires the final factor to be 1.0", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({0, 1, 2}, {0.5, 0.8, 0.9}), Slic3r::InvalidArgument);
}

TEST_CASE("SmallAreaFlowCompensator rejects a small but nonzero first length", "[boss][flow]")
{
    CHECK_THROWS_AS(
        SmallAreaFlowCompensator({1e-5, 1, 2}, {0.5, 0.8, 1.0}),
        Slic3r::InvalidArgument
    );
}

TEST_CASE("SmallAreaFlowCompensator rejects mismatched lengths/factors sizes", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({0, 1, 2}, {0.5, 1.0}), Slic3r::InvalidArgument);
}

TEST_CASE("SmallAreaFlowCompensator rejects empty lengths and factors", "[boss][flow]")
{
    CHECK_THROWS_AS(SmallAreaFlowCompensator({}, {}), Slic3r::InvalidArgument);
}

TEST_CASE(
    "SmallAreaFlowCompensator passes dE through unmodified above the max length",
    "[boss][flow]"
)
{
    auto comp = make_default_compensator();
    CHECK(comp.modify_flow(100.0, 1.0, Slic3r::ExtrusionRole::SolidInfill) == 1.0);
}

TEST_CASE(
    "SmallAreaFlowCompensator passes dE through unmodified for non-solid-infill roles",
    "[boss][flow]"
)
{
    auto comp = make_default_compensator();
    CHECK(comp.modify_flow(0.2, 1.0, Slic3r::ExtrusionRole::InternalInfill) == 1.0);
}

TEST_CASE(
    "SmallAreaFlowCompensator scales dE for short SolidInfill/TopSolidInfill segments",
    "[boss][flow]"
)
{
    auto comp     = make_default_compensator();
    double result = comp.modify_flow(0.2, 1.0, Slic3r::ExtrusionRole::SolidInfill);
    CHECK(result < 1.0);
    CHECK(result > 0.0);
    double top_result = comp.modify_flow(0.2, 1.0, Slic3r::ExtrusionRole::TopSolidInfill);
    CHECK(top_result == result);
}
