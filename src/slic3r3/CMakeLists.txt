project(slic3r3)

include(PrecompiledHeader)

add_subdirectory(Biz)
add_subdirectory(App)
add_subdirectory(Domain)

add_executable(slic3r3 main.cpp)
target_link_libraries(slic3r3 slic3r3_domain slic3r3_biz slic3r3_app)
