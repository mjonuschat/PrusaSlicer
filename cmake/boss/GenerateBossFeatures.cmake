# cmake/boss/GenerateBossFeatures.cmake
#
# Discovers BOSS feature manifests and generates the composition headers
# and per-target source lists that wire them into the build. Called once
# from the top-level CMakeLists.txt; boss_target_sources() is called once
# per augmentable CMake target (slic3r-domain, libslic3r, ...) from that
# target's own CMakeLists.txt. Neither function is edited when a feature
# is added or removed -- only its manifest under
# src/boss/include/boss/features/ is.
#
# BOSS_FEATURES_DIR is overridable so a build can be pointed at an empty or
# synthetic manifest directory without disturbing the real one under
# src/boss/include/boss/features/ -- used by this plan's Task 6 to prove the
# zero-manifest composition state with a real CMake configure and build,
# not only the generator's own standalone unit tests.
set(BOSS_FEATURES_DIR "${CMAKE_SOURCE_DIR}/src/boss/include/boss/features" CACHE PATH
    "Directory of boss-feature.json manifests to discover and compose")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(boss_generate_features)
    set(_output_dir "${CMAKE_BINARY_DIR}/boss-generated")
    set(_generator "${CMAKE_SOURCE_DIR}/cmake/boss/generate_boss_features.py")

    file(GLOB _boss_manifests CONFIGURE_DEPENDS "${BOSS_FEATURES_DIR}/*/boss-feature.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_generator} ${_boss_manifests})

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${_generator}"
                --features-dir "${BOSS_FEATURES_DIR}"
                --output-dir "${_output_dir}"
        RESULT_VARIABLE _boss_generate_result
        ERROR_VARIABLE _boss_generate_error
    )
    if (NOT _boss_generate_result EQUAL 0)
        message(FATAL_ERROR "BOSS feature generation failed:\n${_boss_generate_error}")
    endif ()

    set(BOSS_GENERATED_DIR "${_output_dir}" CACHE INTERNAL "BOSS generated composition output directory")
    set(BOSS_GENERATED_INCLUDE_DIR "${_output_dir}/include" CACHE INTERNAL
        "Shared include root for every generated BOSS composition header")
endfunction()

function(boss_target_sources target_name)
    set(_sources_cmake "${BOSS_GENERATED_DIR}/${target_name}/sources.cmake")
    if (EXISTS "${_sources_cmake}")
        include("${_sources_cmake}")
    endif ()

    set(_vendored_cmake "${BOSS_GENERATED_DIR}/${target_name}/vendored.cmake")
    if (EXISTS "${_vendored_cmake}")
        include("${_vendored_cmake}")
    endif ()

    string(TOUPPER "${target_name}" _target_upper)
    string(REPLACE "-" "_" _target_upper "${_target_upper}")
    set(_sources_var "BOSS_${_target_upper}_SOURCES")
    if (DEFINED ${_sources_var} AND NOT "${${_sources_var}}" STREQUAL "")
        target_sources(${target_name} PRIVATE ${${_sources_var}})
    endif ()

    set(_generated_var "BOSS_${_target_upper}_GENERATED_SOURCES")
    if (DEFINED ${_generated_var} AND NOT "${${_generated_var}}" STREQUAL "")
        target_sources(${target_name} PRIVATE ${${_generated_var}})
    endif ()

    # PUBLIC, not PRIVATE: the shared generated headers and the BOSS
    # include root src/boss/include (for "boss/foundation/..." and
    # "boss/features/..." headers) must reach any target that links
    # against this one -- most immediately, the Catch2 test binaries in
    # Tasks 4-5, which link slic3r-domain/libslic3r but do not call
    # boss_target_sources() themselves.
    #
    # The root is src/boss/include, not the whole src/ tree: a target that
    # links slic3r-domain must not gain an include path to slic3r/GUI or
    # any other higher layer. The BOSS include root exposes BOSS headers
    # and nothing else.
    #
    # A BOSS-added header placed under a target's own private src/ tree by
    # convention (e.g. Task 5's Fill.hpp, next to the Fill.cpp it's
    # extracted from) does NOT need a new include path from this function
    # for tests to reach it: tests/CMakeLists.txt already declares
    # target_include_directories(test_common INTERFACE
    # "$<TARGET_PROPERTY:libslic3r,SOURCE_DIR>/src") specifically "to allow
    # using private header files from libslic3r in all tests", and every
    # test target in this repo already links test_common. Reuse that
    # existing, already-accepted mechanism rather than widening
    # ${target_name}'s own PUBLIC interface with a second, BOSS-specific
    # path to the same directory.
    target_include_directories(${target_name} PUBLIC
        "${CMAKE_SOURCE_DIR}/src/boss/include"
        "${BOSS_GENERATED_INCLUDE_DIR}"
    )
endfunction()
