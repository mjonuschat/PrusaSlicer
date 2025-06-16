///|/ Copyright (c) Prusa Research 2017 - 2021 Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros
///|/
///|/ ported from lib/Slic3r/Format/OBJ.pm:
///|/ Copyright (c) Prusa Research 2017 Vojtěch Bubník @bubnikv
///|/ Copyright (c) Slic3r 2012 - 2014 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <boost/log/trivial.hpp>
#include <string>
#include <utility>
#include <cassert>
#include <cstring>

#include "Slic3r/Biz/Algorithms/Model.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "OBJ.hpp"
#include "objparser.hpp"
#include "admesh/stl.h"

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

using namespace Slic3r::Biz;

namespace Slic3r {

using Domain::TriangleMesh;
namespace TriMesh = Biz::Algorithms::TriangleMesh;

bool load_obj(const char *path, TriangleMesh *meshptr)
{
    if (meshptr == nullptr)
        return false;
    
    // Parse the OBJ file.
    ObjParser::ObjData data;
    if (! ObjParser::objparse(path, data)) {
        BOOST_LOG_TRIVIAL(error) << "load_obj: failed to parse " << path;
        return false;
    }
    
    // Count the faces and verify, that all faces are triangular.
    size_t num_faces = 0;
    size_t num_quads = 0;
    for (size_t i = 0; i < data.vertices.size(); ++ i) {
        // Find the end of face.
        size_t j = i;
        for (; j < data.vertices.size() && data.vertices[j].coordIdx != -1; ++ j) ;
        if (size_t num_face_vertices = j - i; num_face_vertices > 0) {
            if (num_face_vertices > 4) {
                // Non-triangular and non-quad faces are not supported as of now.
                BOOST_LOG_TRIVIAL(error) << "load_obj: failed to parse " << path << ". The file contains polygons with more than 4 vertices.";
                return false;
            } else if (num_face_vertices < 3) {
                // Non-triangular and non-quad faces are not supported as of now.
                BOOST_LOG_TRIVIAL(error) << "load_obj: failed to parse " << path << ". The file contains polygons with less than 2 vertices.";
                return false;
            }
            if (num_face_vertices == 4)
                ++ num_quads;
            ++ num_faces;
            i = j;
        }
    }
    
    // Convert ObjData into indexed triangle set.
    indexed_triangle_set its;
    size_t num_vertices = data.coordinates.size() / 4;
    its.vertices.reserve(num_vertices);
    its.indices.reserve(num_faces + num_quads);
    for (size_t i = 0; i < num_vertices; ++ i) {
        size_t j = i << 2;
        its.vertices.emplace_back(data.coordinates[j], data.coordinates[j + 1], data.coordinates[j + 2]);
    }
    int indices[4];
    for (size_t i = 0; i < data.vertices.size();)
        if (data.vertices[i].coordIdx == -1)
            ++ i;
        else {
            int cnt = 0;
            while (i < data.vertices.size())
                if (const ObjParser::ObjVertex &vertex = data.vertices[i ++]; vertex.coordIdx == -1) {
                    break;
                } else {
                    assert(cnt < 4);
                    if (vertex.coordIdx < 0 || vertex.coordIdx >= int(its.vertices.size())) {
                        BOOST_LOG_TRIVIAL(error) << "load_obj: failed to parse " << path << ". The file contains invalid vertex index.";
                        return false;
                    }
                    indices[cnt ++] = vertex.coordIdx;
                }
            if (cnt) {
                assert(cnt == 3 || cnt == 4);
                // Insert one or two faces (triangulate a quad).
                its.indices.push_back(Domain::Index3{indices[0], indices[1], indices[2]});
                if (cnt == 4)
                    its.indices.push_back(Domain::Index3{indices[0], indices[2], indices[3]});
            }
        }

    using Biz::Algorithms::TriangleMesh::construct;
    *meshptr = TriangleMesh(construct(std::move(its)));
    if (meshptr->empty()) {
        BOOST_LOG_TRIVIAL(error) << "load_obj: This OBJ file couldn't be read because it's empty. " << path;
        return false;
    }
    if (meshptr->volume() < 0)
        meshptr->flip_triangles();
    return true;
}

bool load_obj(const char *path, Domain::Model *model, const char *object_name_in)
{
    TriangleMesh mesh;
    
    bool ret = load_obj(path, &mesh);
    
    if (ret) {
        std::string  object_name;
        if (object_name_in == nullptr) {
            const char *last_slash = strrchr(path, DIR_SEPARATOR);
            object_name.assign((last_slash == nullptr) ? path : last_slash + 1);
        } else
           object_name.assign(object_name_in);
    
        Algorithms::Model::add_object(model, object_name.c_str(), path, std::move(mesh));
    }
    
    return ret;
}

bool store_obj(const char *path, TriangleMesh *mesh)
{
    //FIXME returning false even if write failed.
    TriMesh::write_obj_file(*mesh, path);
    return true;
}

bool store_obj(const char *path, Domain::ModelObject *model_object)
{
    TriangleMesh mesh = Algorithms::ModelObject::mesh(*model_object);
    return store_obj(path, &mesh);
}

bool store_obj(const char *path, Domain::Model *model)
{
    TriangleMesh mesh = Algorithms::Model::mesh(*model);
    return store_obj(path, &mesh);
}

}; // namespace Slic3r
