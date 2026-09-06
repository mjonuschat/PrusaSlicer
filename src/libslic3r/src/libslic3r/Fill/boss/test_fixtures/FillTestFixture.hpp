// src/libslic3r/src/libslic3r/Fill/boss/test_fixtures/FillTestFixture.hpp
//
// Test-only Fill subtype -- delegates entirely to FillRectilinear so the
// dispatch test has a real, distinct type to assert dynamic_cast against,
// without needing any real geometry algorithm.
#pragma once

#include "libslic3r/Fill/FillRectilinear.hpp"

namespace Slic3r {

class FillTestFixture : public FillRectilinear
{
public:
    Fill* clone() const override { return new FillTestFixture(*this); }
};

} // namespace Slic3r
