project(slic3r-domain)

find_package(NanoSVG REQUIRED)

add_library(slic3r-domain
    include/Slic3r/Domain/CustomGCode.hpp
    include/Slic3r/Domain/Line.hpp
    include/Slic3r/Domain/MultiPoint.hpp
    include/Slic3r/Domain/Polygon.hpp
    include/Slic3r/Domain/Polyline.hpp
    src/Slic3r/Domain/Line.cpp
    src/Slic3r/Domain/MultiPoint.cpp
    src/Slic3r/Domain/Polygon.cpp
    src/Slic3r/Domain/Polyline.cpp
    include/Slic3r/Domain/BoundingBox.hpp
    src/Slic3r/Domain/BoundingBox.cpp
    include/Slic3r/Domain/Point.hpp
    src/Slic3r/Domain/Point.cpp

    include/Slic3r/Domain/ExPolygon.hpp
    src/Slic3r/Domain/ExPolygon.cpp
    include/Slic3r/Domain/PrinterTechnology.hpp

    include/Slic3r/Domain/Expr/ExprAst.hpp
    include/Slic3r/Domain/Expr/RegEx.hpp

    include/Slic3r/Domain/Preset/PresetTree.hpp
    include/Slic3r/Domain/Preset/SourceLocatedExpr.hpp
    include/Slic3r/Domain/Preset/Types.hpp
    include/Slic3r/Domain/Preset/HwConfig.hpp
    include/Slic3r/Domain/Preset/EvaluatedPreset.hpp
    include/Slic3r/Domain/Preset/Bundle.hpp
    src/Slic3r/Domain/Preset/HwConfig.cpp
    src/Slic3r/Domain/Preset/PresetTree.cpp
    src/Slic3r/Domain/Preset/EvaluatedPreset.cpp

    include/Slic3r/Domain/FacetsAnnotation.hpp
    src/Slic3r/Domain/FacetsAnnotation.cpp

    include/Slic3r/Domain/ObjectID.hpp
    src/Slic3r/Domain/ObjectID.cpp

    include/Slic3r/Domain/TriangleMesh.hpp
    src/Slic3r/Domain/TriangleMesh.cpp

    include/Slic3r/Domain/TriangleSelector.hpp
    src/Slic3r/Domain/TriangleSelector.cpp

    include/Slic3r/Domain/SLA/DrainHole.hpp
    src/Slic3r/Domain/SLA/DrainHole.cpp

    include/Slic3r/Domain/SLA/SupportPoint.hpp
    src/Slic3r/Domain/SLA/SupportPoint.cpp

    include/Slic3r/Domain/TextConfiguration.hpp
    src/Slic3r/Domain/TextConfiguration.cpp

    include/Slic3r/Domain/EmbossShape.hpp

    include/Slic3r/Domain/SelectionId.hpp

    include/Slic3r/Domain/ElementRef.hpp
    src/Slic3r/Domain/ElementId.cpp

    include/Slic3r/Domain/Bed.hpp
    src/Slic3r/Domain/Bed.cpp

    include/Slic3r/Domain/BedRef.hpp

    include/Slic3r/Domain/BedInstance.hpp
    src/Slic3r/Domain/BedInstance.cpp

    include/Slic3r/Domain/Transformation.hpp
    src/Slic3r/Domain/Transformation.cpp
)

target_include_directories(slic3r-domain PUBLIC include)
target_include_directories(slic3r-domain PRIVATE src)


target_link_libraries(slic3r-domain PUBLIC
    admesh # TODO, remove dependency on admesh by refactoring triangle mesh
    NanoSVG::nanosvg # TODO remove dependency on nanosvg by refactoring EmbossShape
    NanoSVG::nanosvgrast
)

target_link_libraries(slic3r-domain PUBLIC slic3r-base TBB::tbbmalloc)


if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-domain-tests
            test/Slic3r/Domain/Preset/PresetTreeTests.cpp
            test/Slic3r/Domain/Preset/HwConfigTests.cpp

    )
    target_include_directories(slic3r-domain-tests PRIVATE test src)

    find_package(Catch2 3.8 REQUIRED)
    slic3r_remap_configs("Catch2::Catch2" RelWithDebInfo Release)
    target_link_libraries(slic3r-domain-tests PRIVATE slic3r-domain Catch2::Catch2WithMain)

    set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/data)
    file(TO_NATIVE_PATH "${TEST_DATA_DIR}" TEST_DATA_DIR)
    target_compile_definitions(slic3r-domain-tests PRIVATE TEST_DATA_DIR=R"\(${TEST_DATA_DIR}\)")

    set(_catch_args "exclude:[NotWorking] exclude:[Slow]")
    list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
    add_test(NAME slic3r-domain-tests
            COMMAND slic3r-domain-tests ${_catch_args}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if (WIN32)
        prusaslicer_copy_dlls(slic3r-domain-tests)
    endif ()
endif()



