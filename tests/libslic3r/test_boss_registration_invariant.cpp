#include <catch2/catch_test_macros.hpp>

#include <set>
#include <stdexcept>
#include <string>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "boss/foundation/BossRegistrationInvariant.hpp"

using Slic3r::Boss::assert_boss_registration_matches_manifest;

TEST_CASE("matching key sets pass", "[boss][invariant]")
{
    REQUIRE_NOTHROW(assert_boss_registration_matches_manifest({"a", "b"}, {"a", "b"}));
}

TEST_CASE("a registered key with no manifest entry throws", "[boss][invariant]")
{
    REQUIRE_THROWS_AS(assert_boss_registration_matches_manifest({"a", "b"}, {"a"}), std::logic_error);
}

TEST_CASE("a manifest entry with no registration throws", "[boss][invariant]")
{
    REQUIRE_THROWS_AS(assert_boss_registration_matches_manifest({"a"}, {"a", "b"}), std::logic_error);
}

TEST_CASE("fdm config definitions construct with the BOSS invariant satisfied", "[boss][invariant]")
{
    REQUIRE_NOTHROW(Slic3r::Domain::get_defs_fdm());
}
