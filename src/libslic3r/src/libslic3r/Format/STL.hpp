///|/ Copyright (c) Prusa Research 2017 - 2019 Vojtěch Bubník @bubnikv
///|/
///|/ ported from lib/Slic3r/Format/STL.pm:
///|/ Copyright (c) Prusa Research 2017 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Format_STL_hpp_
#define slic3r_Format_STL_hpp_

#include "Slic3r/Domain/TriangleMesh.hpp"

namespace Slic3r::Domain {
class Model;
class ModelObject;
} // namespace Slic3r::Domain

namespace Slic3r {

// Load an STL file into a provided model.
extern bool load_stl(const char *path, Domain::Model *model, const char *object_name = nullptr);

extern bool store_stl(const char *path, Domain::TriangleMesh *mesh, bool binary);
extern bool store_stl(const char *path, Domain::ModelObject *model_object, bool binary);
extern bool store_stl(const char *path, Domain::Model *model, bool binary);

}; // namespace Slic3r

#endif /* slic3r_Format_STL_hpp_ */
