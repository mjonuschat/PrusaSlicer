// src/libslic3r/src/libslic3r/Fill/boss/test_fixtures/FillTestFixture.cpp
#include "boss/test-fixtures/fill-fixture/FillFixtureFeature.hpp"
#include "libslic3r/Fill/boss/test_fixtures/FillTestFixture.hpp"

namespace Slic3r::Boss::Test {

std::unique_ptr<Fill> FillFixtureFeature::create_fill()
{
    return std::make_unique<FillTestFixture>();
}

bool FillFixtureFeature::use_bridge_flow()
{
    return false;
}

} // namespace Slic3r::Boss::Test
