///|/ Copyright (c) Prusa Research 2017 - 2019 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv
///|/
///|/ ported from lib/Slic3r/Format/OBJ.pm:
///|/ Copyright (c) Prusa Research 2017 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2012 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/TriangleMesh.hpp"
#include "tl/expected.hpp"

namespace Slic3r::Domain {
    class Model;
}

namespace Slic3r::Biz {

tl::expected<Domain::TriangleMesh, std::string> load_obj(const std::string& path);
bool store_obj(const std::string& path, const Domain::TriangleMesh& mesh);
bool store_obj(const std::string& path, Domain::Model* model);

}; // namespace Slic3r::Biz
