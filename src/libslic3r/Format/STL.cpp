///|/ Copyright (c) Prusa Research 2017 - 2021 Vojtěch Bubník @bubnikv
///|/
///|/ ported from lib/Slic3r/Format/STL.pm:
///|/ Copyright (c) Prusa Research 2017 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2011 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2012 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <string>
#include <utility>
#include <cstring>

#include "Slic3r/Biz/Algorithms/Model.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "STL.hpp"

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

using namespace Slic3r::Biz;

namespace Slic3r {

using Domain::TriangleMesh;
namespace TriMesh = Biz::Algorithms::TriangleMesh;

bool load_stl(const char* path, Domain::Model* model, const char* object_name_in)
{
    std::optional<TriangleMesh> mesh{TriMesh::read_stl_file(path)};
    if (!mesh) {
        //    die "Failed to open $file\n" if !-e $path;
        return false;
    }
    if (mesh->empty()) {
        // die "This STL file couldn't be read because it's empty.\n"
        return false;
    }

    std::string object_name;
    if (object_name_in == nullptr) {
        const char* last_slash = strrchr(path, DIR_SEPARATOR);
        object_name.assign((last_slash == nullptr) ? path : last_slash + 1);
    } else {
        object_name.assign(object_name_in);
    }

    Algorithms::Model::add_object(model, object_name.c_str(), path, std::move(*mesh));
    return true;
}

bool store_stl(const char *path, TriangleMesh *mesh, bool binary)
{
    if (binary)
        TriMesh::write_binary(*mesh, path);
    else
        TriMesh::write_ascii(*mesh, path);
    //FIXME returning false even if write failed.
    return true;
}

bool store_stl(const char *path, Domain::ModelObject *model_object, bool binary)
{
    TriangleMesh mesh = Algorithms::ModelObject::mesh(*model_object);
    return store_stl(path, &mesh, binary);
}

bool store_stl(const char *path, Domain::Model *model, bool binary)
{
    TriangleMesh mesh = Algorithms::Model::mesh(*model);
    return store_stl(path, &mesh, binary);
}

}; // namespace Slic3r
