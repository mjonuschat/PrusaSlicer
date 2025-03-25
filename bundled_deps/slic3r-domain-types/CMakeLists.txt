add_library(slic3r-domain-types INTERFACE)
target_include_directories(slic3r-domain-types INTERFACE include)
target_link_libraries(slic3r-domain-types INTERFACE Eigen3::Eigen)
