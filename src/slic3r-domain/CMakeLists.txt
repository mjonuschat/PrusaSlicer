project(slic3r-domain)

add_library(slic3r-domain
    include/Slic3r/Domain/Line.hpp
    include/Slic3r/Domain/MultiPoint.hpp
    include/Slic3r/Domain/Polygon.hpp
    include/Slic3r/Domain/Polyline.hpp
    include/Slic3r/Domain/Vectors.hpp
    src/Slic3r/Domain/Line.cpp
    src/Slic3r/Domain/MultiPoint.cpp
    src/Slic3r/Domain/Polygon.cpp
    src/Slic3r/Domain/Polyline.cpp
    src/Slic3r/Domain/Vectors.cpp
    include/Slic3r/Domain/BoundingBox.hpp
    src/Slic3r/Domain/BoundingBox.cpp
    include/Slic3r/Domain/Point.hpp
    src/Slic3r/Domain/Point.cpp

    include/Slic3r/Domain/ExPolygon.hpp
    src/Slic3r/Domain/ExPolygon.cpp
)

target_include_directories(slic3r-domain PUBLIC include)
target_include_directories(slic3r-domain PRIVATE src)

target_link_libraries(slic3r-domain PUBLIC Eigen3::Eigen slic3r-base TBB::tbbmalloc)
