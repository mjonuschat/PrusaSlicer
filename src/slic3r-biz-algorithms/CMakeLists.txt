project(slic3r-biz-algorithms)

add_library(slic3r-biz-algorithms
    include/Slic3r/Biz/Algorithms/Scaling.hpp
    src/Slic3r/Biz/Algorithms/Scaling.cpp
)
target_include_directories(slic3r-biz-algorithms PUBLIC include)
target_include_directories(slic3r-biz-algorithms PRIVATE src)

target_link_libraries(slic3r-biz-algorithms PUBLIC slic3r-domain slic3r-base)

if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-biz-algorithms-tests
        test/Slic3r/Biz/Algorithms/ScalingTests.cpp
        test/Slic3r/Biz/Algorithms/BoundingBoxTests.cpp
    )
    target_include_directories(slic3r-biz-algorithms-tests PRIVATE test)

    find_package(Catch2 3.8 REQUIRED)
    slic3r_remap_configs("Catch2::Catch2" RelWithDebInfo Release)
    target_link_libraries(slic3r-biz-algorithms-tests PRIVATE slic3r-biz-algorithms Catch2::Catch2WithMain)

    set(_catch_args "exclude:[NotWorking] exclude:[Slow]")
    list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
    add_test(NAME slic3r-biz-algorithms-tests
        COMMAND slic3r-biz-algorithms-tests ${_catch_args}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
