// src/libslic3r/src/libslic3r/boss/test_fixtures/CapabilitylessFixture.cpp
#include "boss/test-fixtures/capabilityless-fixture/CapabilitylessFixtureFeature.hpp"
#include "libslic3r/boss/test_fixtures/CapabilitylessFixture.hpp"

namespace Slic3r::Boss::Test {

int capabilityless_fixture_marker()
{
    return CapabilitylessFixtureFeature::id;
}

} // namespace Slic3r::Boss::Test
