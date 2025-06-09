///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Slic3r/App/Yoga/Validator.hpp>

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;

TEST_CASE("IntValidator simple")
{
    IntValidator validator;

    REQUIRE(validator.process("10") == "10");
}

TEST_CASE("IntValidator zero")
{
    IntValidator validator;

    REQUIRE(validator.process("0") == "0");
}

TEST_CASE("IntValidator big")
{
    IntValidator validator;

    REQUIRE(validator.process("123456789") == "123456789");
}

TEST_CASE("IntValidator negative")
{
    IntValidator validator;

    REQUIRE(validator.process("-999") == "-999");
}

TEST_CASE("IntValidator range")
{
    IntValidator validator(0, 100);

    REQUIRE(validator.process("-999") == "0");
    REQUIRE(validator.process("999") == "100");
    REQUIRE(validator.process("50") == "50");
}

TEST_CASE("IntValidator close range")
{
    IntValidator validator(-1, -1);

    REQUIRE(validator.process("-1") == "-1");
    REQUIRE(validator.process("-5") == "-1");
    REQUIRE(validator.process("1") == "-1");
}

TEST_CASE("IntValidator complex")
{
    IntValidator validator;

    REQUIRE(validator.process("-1 + 50") == "49");
    REQUIRE(validator.process("0+1+2+3+4+5") == "15");
    REQUIRE(validator.process("2*2") == "4");
    REQUIRE(validator.process("2/2") == "1");
    REQUIRE(validator.process("2/2+1+2") == "4");
    REQUIRE(validator.process("2.5*2") == "5");
}

TEST_CASE("DoubleValidator simple")
{
    DoubleValidator validator;

    REQUIRE_THAT(std::stod(validator.process("0")), WithinRel(0., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-999")), WithinRel(-999., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-999.999")), WithinRel(-999.999, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-0.1")), WithinRel(-0.1, 0.0001));
}

TEST_CASE("DoubleValidator complex")
{
    DoubleValidator validator;

    REQUIRE_THAT(std::stod(validator.process("-1 + 50")), WithinRel(49., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("0+1+2+3+4+5")), WithinRel(15., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("2*2")), WithinRel(4, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("2/2")), WithinRel(1, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("2/2+1+2")), WithinRel(4, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("2.5*2")), WithinRel(5, 0.0001));
}

TEST_CASE("DoubleValidator range")
{
    DoubleValidator validator(-5, 0.5);

    REQUIRE_THAT(std::stod(validator.process("0")), WithinRel(0., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-999")), WithinRel(-5., 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-999.999")), WithinRel(-5, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("-0.1")), WithinRel(-0.1, 0.0001));
    REQUIRE_THAT(std::stod(validator.process("5.1")), WithinRel(0.5, 0.0001));
}
