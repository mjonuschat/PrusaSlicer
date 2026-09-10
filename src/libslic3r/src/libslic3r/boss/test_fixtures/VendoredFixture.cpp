// src/libslic3r/src/libslic3r/boss/test_fixtures/VendoredFixture.cpp
#include "libslic3r/boss/test_fixtures/VendoredFixture.hpp"

#include "VendoredFixtureVendor.hpp"

namespace Slic3r::Boss::Test {

int vendored_fixture_marker()
{
    return vendored_fixture_vendor_marker;
}

} // namespace Slic3r::Boss::Test
