project(slic3r-domain)

add_library(slic3r-domain
    include/Slic3r/Domain/Vectors.hpp
    src/Slic3r/Domain/Vectors.cpp
)

target_include_directories(slic3r-domain PUBLIC include)
target_include_directories(slic3r-domain PRIVATE src)

target_link_libraries(slic3r-domain PUBLIC Eigen3::Eigen slic3r-base)
