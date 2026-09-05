#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "libslic3r/TriangleMeshSlicer.hpp"
#include "libslic3r/Polygon.hpp"

using namespace Slic3r;

namespace {

indexed_triangle_set make_frame_with_hole()
{
    return {
        { {0,1,2}, {2,1,3}, {1,0,4}, {5,1,4}, {6,7,4}, {8,2,9}, {0,2,8}, {10,8,9},
          {0,8,6}, {0,6,4}, {4,7,9}, {7,10,9}, {2,3,9}, {9,3,11}, {12,1,5}, {13,3,12},
          {14,12,5}, {3,1,12}, {11,3,13}, {11,15,5}, {11,13,15}, {15,14,5}, {5,4,9},
          {11,5,9}, {8,13,12}, {6,8,12}, {10,15,13}, {8,10,13}, {15,10,14}, {14,10,7},
          {14,7,12}, {12,7,6} },
        { {0.f,0.f,0.f}, {0.f,0.f,10.f}, {0.f,20.f,0.f}, {0.f,20.f,10.f}, {20.f,0.f,0.f},
          {20.f,0.f,10.f}, {5.f,5.f,0.f}, {15.f,5.f,0.f}, {5.f,15.f,0.f}, {20.f,20.f,0.f},
          {15.f,15.f,0.f}, {20.f,20.f,10.f}, {5.f,5.f,10.f}, {5.f,15.f,10.f}, {15.f,5.f,10.f},
          {15.f,15.f,10.f} }
    };
}

} // namespace

TEST_CASE("project_mesh preserves the hole in a frame shape", "[ProjectMesh]")
{
    const indexed_triangle_set its = make_frame_with_hole();
    const Domain::Polygons projected = project_mesh(its, Domain::Transform3d::Identity(), [] {});

    const size_t hole_count = std::count_if(
        projected.begin(), projected.end(), [](const Domain::Polygon &p) { return p.area() < 0; }
    );
    REQUIRE(hole_count == 1);
}
