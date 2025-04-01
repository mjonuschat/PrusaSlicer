project(slic3r-biz-algorithms)

add_library(slic3r-biz-algorithms
    include/Slic3r/Biz/Algorithms/BoundingBox.hpp
    src/Slic3r/Biz/Algorithms/BoundingBox.cpp
    include/Slic3r/Biz/Algorithms/DouglasPeucker.hpp
    src/Slic3r/Biz/Algorithms/DouglasPeucker.cpp
    include/Slic3r/Biz/Algorithms/ExPolygon.hpp
    src/Slic3r/Biz/Algorithms/ExPolygon.cpp
    src/Slic3r/Biz/Algorithms/Scaling.cpp
    include/Slic3r/Biz/Algorithms/Line.hpp
    src/Slic3r/Biz/Algorithms/Line.cpp
    include/Slic3r/Biz/Algorithms/MultiPoint.hpp
    src/Slic3r/Biz/Algorithms/MultiPoint.cpp
    include/Slic3r/Biz/Algorithms/Point.hpp
    src/Slic3r/Biz/Algorithms/Point.cpp
    include/Slic3r/Biz/Algorithms/Polygon.hpp
    src/Slic3r/Biz/Algorithms/Polygon.cpp
    include/Slic3r/Biz/Algorithms/Polyline.hpp
    src/Slic3r/Biz/Algorithms/Polyline.cpp
    include/Slic3r/Biz/Algorithms/Scaling.hpp
    src/Slic3r/Biz/Algorithms/Scaling.cpp
    include/Slic3r/Biz/Algorithms/SVG.hpp
    src/Slic3r/Biz/Algorithms/SVG.cpp
    include/Slic3r/Biz/Algorithms/TravelingSalesman.hpp
    src/Slic3r/Biz/Algorithms/TravelingSalesman.cpp
    include/Slic3r/Biz/Algorithms/ClipperUtils.hpp
    src/Slic3r/Biz/Algorithms/ClipperUtils.cpp
    include/Slic3r/Biz/Algorithms/KDTreeIndirect.hpp
    include/Slic3r/Biz/Algorithms/MutablePriorityQueue.hpp
)
target_include_directories(slic3r-biz-algorithms PUBLIC include)
target_include_directories(slic3r-biz-algorithms PRIVATE src)

target_link_libraries(slic3r-biz-algorithms PUBLIC slic3r-domain slic3r-base clipper)

if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-biz-algorithms-tests
        test/Slic3r/Biz/Algorithms/ScalingTests.cpp
        test/Slic3r/Biz/Algorithms/BoundingBoxTests.cpp
    )
    target_include_directories(slic3r-biz-algorithms-tests PRIVATE test)

    find_package(Catch2 3.8 REQUIRED)
    slic3r_remap_configs(Catch2::Catch2 RelWithDebInfo Release)
    slic3r_remap_configs(Catch2::Catch2WithMain RelWithDebInfo Release)

    target_link_libraries(slic3r-biz-algorithms-tests PRIVATE slic3r-biz-algorithms Catch2::Catch2WithMain TBB::tbb)

    set(_catch_args "exclude:[NotWorking] exclude:[Slow]")
    list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
    add_test(NAME slic3r-biz-algorithms-tests
        COMMAND slic3r-biz-algorithms-tests ${_catch_args}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
