#ifndef SLIC3R_TEST_UTILS
#define SLIC3R_TEST_UTILS

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "libslic3r/libslic3r.h"
#include <libslic3r/Format/OBJ.hpp>
#include <random>

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR R"(\)"
#else
#define PATH_SEPARATOR R"(/)"
#endif

inline Slic3r::Domain::TriangleMesh load_model(const std::string &obj_filename)
{
    Slic3r::Domain::TriangleMesh mesh;
    auto fpath = TEST_DATA_DIR PATH_SEPARATOR + obj_filename;
    Slic3r::load_obj(fpath.c_str(), &mesh);
    return mesh;
}

template<class T>
Slic3r::FloatingOnly<T> random_value(T minv, T maxv)
{
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<T> dist(minv, maxv);

    return dist(rng);
}

template<class T>
Slic3r::IntegerOnly<T> random_value(T minv, T maxv)
{
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<T> dist(minv, maxv);

    return dist(rng);
}

#endif // SLIC3R_TEST_UTILS
